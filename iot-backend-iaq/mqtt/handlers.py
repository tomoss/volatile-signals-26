"""Application-level handling for messages received from IAQ devices."""

import json
import logging
from collections import namedtuple
from collections.abc import Callable
from dataclasses import dataclass
from datetime import UTC, datetime
from functools import lru_cache

from django.db import IntegrityError, transaction
from django.db.models import Q

from iaq.models import Device, DeviceInfo, DeviceStatus, HealthReading, SensorReading
from mqtt import topics

logger = logging.getLogger(__name__)

Handler = Callable[[str, bytes], None]

ParsedTopic = namedtuple("ParsedTopic", ["mac", "message_type"])


@dataclass
class SensorPayload:
    iaq: float
    co2: float
    voc: float
    temp: float
    hum: float
    pressure: float
    iaq_accuracy: int
    timestamp: int

    @classmethod
    def fromJsonBytes(cls, payload: bytes) -> "SensorPayload":
        return cls(**json.loads(payload))


@dataclass
class HealthPayload:
    rssi: int
    heap: int
    min_heap: int
    uptime: int
    timestamp: int

    @classmethod
    def fromJsonBytes(cls, payload: bytes) -> "HealthPayload":
        return cls(**json.loads(payload))


@dataclass
class InfoPayload:
    firmware_version: str
    chip_model: str
    chip_revision: int
    chip_cores: int
    reset_reason: int
    total_heap: int

    @classmethod
    def fromJsonBytes(cls, payload: bytes) -> "InfoPayload":
        return cls(**json.loads(payload))


class MessageHandler:
    """Validate and persist messages received on IAQ MQTT topics."""

    def __init__(self):
        self._handlers: dict[str, Handler] = {
            topics.SENSOR_TOPIC_SUFFIX: self._handle_sensor,
            topics.HEALTH_TOPIC_SUFFIX: self._handle_health,
            topics.INFO_TOPIC_SUFFIX: self._handle_info,
            topics.STATUS_TOPIC_SUFFIX: self._handle_status,
        }

    def on_message(self, client, userdata, message) -> None:
        topic = message.topic
        payload = message.payload
        try:
            parsed_topic = self._parse_topic(topic)
            handler = self._handlers.get(
                parsed_topic.message_type, self._handle_unknown
            )
            handler(parsed_topic.mac, payload)
        except (
            ValueError,
            TypeError,
            UnicodeDecodeError,
            json.JSONDecodeError,
            Device.DoesNotExist,
            IntegrityError,
        ) as exc:
            logger.warning("Failed to process MQTT message: %s", exc)

    def _parse_topic(self, topic: str) -> ParsedTopic:
        left, middle, right = topic.split("/", 2)
        if left != "iaq" or not middle or not right:
            raise ValueError(f"malformed topic: {topic!r}")
        return ParsedTopic(mac=middle, message_type=right)

    def _handle_unknown(self, mac: str, payload: bytes) -> None:
        logger.warning(
            "No handler available for this message type from device: %s", mac
        )

    @staticmethod
    @lru_cache(maxsize=10)
    def _get_device_id(mac: str) -> int:
        return Device.objects.only("id").get(mac=mac).id

    def _handle_sensor(self, mac: str, payload: bytes) -> None:
        data = SensorPayload.fromJsonBytes(payload)
        device_id = MessageHandler._get_device_id(mac)
        timestamp = datetime.fromtimestamp(data.timestamp, tz=UTC)

        with transaction.atomic():
            sensor_reading = SensorReading.objects.create(
                device_id=device_id,
                iaq=data.iaq,
                co2_equivalent=data.co2,
                voc_equivalent=data.voc,
                temperature=data.temp,
                humidity=data.hum,
                pressure=data.pressure,
                accuracy=data.iaq_accuracy,
                timestamp=timestamp,
            )
            Device.objects.filter(pk=device_id).filter(
                Q(latest_sensor_reading__isnull=True)
                | Q(latest_sensor_reading__timestamp__lte=timestamp)
            ).update(latest_sensor_reading=sensor_reading)

    def _handle_health(self, mac: str, payload: bytes) -> None:
        data = HealthPayload.fromJsonBytes(payload)
        device_id = MessageHandler._get_device_id(mac)
        timestamp = datetime.fromtimestamp(data.timestamp, tz=UTC)

        with transaction.atomic():
            health_reading = HealthReading.objects.create(
                device_id=device_id,
                rssi=data.rssi,
                heap=data.heap,
                min_heap=data.min_heap,
                uptime=data.uptime,
                timestamp=timestamp,
            )
            Device.objects.filter(pk=device_id).filter(
                Q(latest_health_reading__isnull=True)
                | Q(latest_health_reading__timestamp__lte=timestamp)
            ).update(latest_health_reading=health_reading)

    def _handle_info(self, mac: str, payload: bytes) -> None:
        data = InfoPayload.fromJsonBytes(payload)
        device_id = MessageHandler._get_device_id(mac)

        DeviceInfo.objects.update_or_create(
            device_id=device_id,
            defaults={
                "firmware_version": data.firmware_version,
                "chip_model": data.chip_model,
                "chip_revision": data.chip_revision,
                "chip_cores": data.chip_cores,
                "reset_reason": data.reset_reason,
                "total_heap": data.total_heap,
            },
        )

    def _handle_status(self, mac: str, payload: bytes) -> None:
        status = payload.decode("utf-8").strip().lower()

        if status not in ("online", "offline"):
            raise ValueError(f"unknown status payload from device {mac}: {status}")

        device_id = MessageHandler._get_device_id(mac)

        DeviceStatus.objects.update_or_create(
            device_id=device_id,
            defaults={"is_online": status == "online"},
        )
