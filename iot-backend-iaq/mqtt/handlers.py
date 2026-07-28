"""Application-level handling for messages received from IAQ devices."""

import json
import logging
from collections.abc import Callable
from dataclasses import dataclass
from datetime import UTC, datetime
from typing import Any, NamedTuple

from django.db import IntegrityError, transaction
from django.db.models import Q

from iaq.models import Device, DeviceInfo, DeviceStatus, HealthReading, SensorReading
from mqtt import topics

logger = logging.getLogger(__name__)

Handler = Callable[[int, str, bytes], None]


class PayloadError(Exception):
    """Raised when an MQTT topic or payload is missing a field or has the wrong type."""


class ParsedTopic(NamedTuple):
    mac: str
    message_type: str


def _get_number(data: dict[str, Any], name: str) -> float:
    value = data.get(name)
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        raise PayloadError(f"field {name!r} must be a number")
    return float(value)


def _get_int(data: dict[str, Any], name: str) -> int:
    value = data.get(name)
    if not isinstance(value, int) or isinstance(value, bool):
        raise PayloadError(f"field {name!r} must be an integer")
    return value


def _get_str(data: dict[str, Any], name: str) -> str:
    value = data.get(name)
    if not isinstance(value, str):
        raise PayloadError(f"field {name!r} must be a string")
    return value


def _get_timestamp(data: dict[str, Any], name: str) -> datetime:
    return datetime.fromtimestamp(_get_int(data, name), tz=UTC)


def _parse_json_object(payload: bytes) -> dict[str, Any]:
    data = json.loads(payload)
    if not isinstance(data, dict):
        raise PayloadError(f"payload must be a JSON object, got {type(data).__name__}")
    return data


@dataclass
class SensorPayload:
    iaq: float
    co2: float
    voc: float
    temp: float
    hum: float
    pressure: float
    iaq_accuracy: int
    timestamp: datetime

    @classmethod
    def parse(cls, payload: bytes) -> "SensorPayload":
        data = _parse_json_object(payload)
        return cls(
            iaq=_get_number(data, "iaq"),
            co2=_get_number(data, "co2"),
            voc=_get_number(data, "voc"),
            temp=_get_number(data, "temp"),
            hum=_get_number(data, "hum"),
            pressure=_get_number(data, "pressure"),
            iaq_accuracy=_get_int(data, "iaq_accuracy"),
            timestamp=_get_timestamp(data, "timestamp"),
        )


@dataclass
class HealthPayload:
    rssi: int
    heap: int
    min_heap: int
    uptime: int
    timestamp: datetime

    @classmethod
    def parse(cls, payload: bytes) -> "HealthPayload":
        data = _parse_json_object(payload)
        return cls(
            rssi=_get_int(data, "rssi"),
            heap=_get_int(data, "heap"),
            min_heap=_get_int(data, "min_heap"),
            uptime=_get_int(data, "uptime"),
            timestamp=_get_timestamp(data, "timestamp"),
        )


@dataclass
class InfoPayload:
    firmware_version: str
    chip_model: str
    chip_revision: int
    chip_cores: int
    reset_reason: int
    total_heap: int

    @classmethod
    def parse(cls, payload: bytes) -> "InfoPayload":
        data = _parse_json_object(payload)
        return cls(
            firmware_version=_get_str(data, "firmware_version"),
            chip_model=_get_str(data, "chip_model"),
            chip_revision=_get_int(data, "chip_revision"),
            chip_cores=_get_int(data, "chip_cores"),
            reset_reason=_get_int(data, "reset_reason"),
            total_heap=_get_int(data, "total_heap"),
        )


class MessageHandler:
    """Validate and persist messages received on IAQ MQTT topics."""

    def __init__(self):
        self._device_cache: dict[str, int] = {}

    def handle(self, topic: str, payload: bytes) -> None:
        try:
            parsed_topic = self._parse_topic(topic)
            match parsed_topic.message_type:
                case topics.SENSOR_TOPIC_NAME:
                    self._handle_sensor(parsed_topic.mac, payload)
                case topics.HEALTH_TOPIC_NAME:
                    self._handle_health(parsed_topic.mac, payload)
                case topics.INFO_TOPIC_NAME:
                    self._handle_info(parsed_topic.mac, payload)
                case topics.STATUS_TOPIC_NAME:
                    self._handle_status(parsed_topic.mac, payload)
                case _:
                    logger.warning("Unknown MQTT message type in topic: %s", topic)

        except PayloadError as exc:
            logger.warning("Failed to process MQTT message on topic %s: %s", topic, exc)

    def _parse_topic(self, topic: str) -> ParsedTopic:
        parts = topic.split("/", 2)
        if len(parts) != 3 or parts[0] != "iaq" or not parts[1]:
            raise PayloadError(f"malformed topic: {topic!r}")

        return ParsedTopic(mac=parts[1], message_type=parts[2])

    def _get_device_id(self, mac: str) -> int:
        device_id = self._device_cache.get(mac)
        if device_id is not None:
            return device_id

        try:
            device_id = Device.objects.only("id").get(mac=mac).id
        except Device.DoesNotExist as exc:
            raise PayloadError(f"unknown device: {mac}") from exc

        self._device_cache[mac] = device_id
        return device_id

    def _write_with_device_guard(
        self, mac: str, device_id: int, write: Callable[[], None]
    ) -> None:
        try:
            with transaction.atomic():
                write()
        except IntegrityError:
            if not Device.objects.filter(pk=device_id).exists():
                logger.warning("Device %s was deleted; removing from cache", mac)
                self._device_cache.pop(mac, None)
            else:
                raise

    def _handle_sensor(self, mac: str, payload: bytes) -> None:
        try:
            data = SensorPayload.parse(payload)
        except json.JSONDecodeError as exc:
            raise PayloadError(f"invalid sensor payload from device {mac}: {exc}") from exc

        device_id = self._get_device_id(mac)

        def write() -> None:
            sensor_reading = SensorReading.objects.create(
                device_id=device_id,
                iaq=data.iaq,
                co2_equivalent=data.co2,
                voc_equivalent=data.voc,
                temperature=data.temp,
                humidity=data.hum,
                pressure=data.pressure,
                accuracy=data.iaq_accuracy,
                timestamp=data.timestamp,
            )
            Device.objects.filter(pk=device_id).filter(
                Q(latest_reading__isnull=True)
                | Q(latest_reading__timestamp__lte=data.timestamp)
            ).update(latest_reading=sensor_reading)

        self._write_with_device_guard(mac, device_id, write)

    def _handle_health(self, mac: str, payload: bytes) -> None:
        try:
            data = HealthPayload.parse(payload)
        except json.JSONDecodeError as exc:
            raise PayloadError(f"invalid health payload from device {mac}: {exc}") from exc

        device_id = self._get_device_id(mac)

        def write() -> None:
            HealthReading.objects.create(
                device_id=device_id,
                rssi=data.rssi,
                heap=data.heap,
                min_heap=data.min_heap,
                uptime=data.uptime,
                timestamp=data.timestamp,
            )

        self._write_with_device_guard(mac, device_id, write)

    def _handle_info(self, mac: str, payload: bytes) -> None:
        try:
            data = InfoPayload.parse(payload)
        except json.JSONDecodeError as exc:
            raise PayloadError(f"invalid info payload from device {mac}: {exc}") from exc

        device_id = self._get_device_id(mac)

        def write() -> None:
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

        self._write_with_device_guard(mac, device_id, write)

    def _handle_status(self, mac: str, payload: bytes) -> None:
        try:
            status = payload.decode("utf-8").strip().lower()
        except UnicodeDecodeError as exc:
            raise PayloadError(f"invalid status payload from device {mac}: {exc}") from exc

        if status not in ("online", "offline"):
            raise PayloadError(f"unknown status payload from device {mac}: {status}")

        device_id = self._get_device_id(mac)

        def write() -> None:
            DeviceStatus.objects.update_or_create(
                device_id=device_id,
                defaults={"is_online": status == "online"},
            )

        self._write_with_device_guard(mac, device_id, write)
