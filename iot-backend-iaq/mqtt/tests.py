import json
from datetime import UTC, datetime

from django.contrib.auth import get_user_model
from django.test import TestCase

from iaq.models import Device, DeviceInfo, DeviceStatus, HealthReading, SensorReading
from mqtt.handlers import MessageHandler


class FakeMessage:
    def __init__(self, topic: str, payload: bytes):
        self.topic = topic
        self.payload = payload


def to_bytes(payload: str) -> bytes:
    return payload.encode("utf-8")


class MessageHandlerTests(TestCase):
    def setUp(self):
        MessageHandler._get_device_id.cache_clear()

        user = get_user_model().objects.create_user(
            email="device-owner@example.com", password="irrelevant"
        )
        self.device = Device.objects.create(user=user, mac="AA:BB:CC:DD:EE:FF")
        self.handler = MessageHandler()

    def test_sensor_creates_reading_and_updates_latest(self):
        payload = json.dumps(
            {
                "iaq": 50.0,
                "co2": 400.0,
                "voc": 0.5,
                "temp": 21.5,
                "hum": 45.0,
                "pressure": 1013.0,
                "iaq_accuracy": 3,
                "timestamp": 1700000000,
            }
        )
        self.handler.on_message(
            None,
            None,
            FakeMessage(f"iaq/{self.device.mac}/sensor_data", to_bytes(payload)),
        )

        reading = SensorReading.objects.get()
        self.assertEqual(reading.device_id, self.device.id)
        self.assertEqual(reading.iaq, 50.0)
        self.assertEqual(reading.co2_equivalent, 400.0)

        self.device.refresh_from_db()
        self.assertEqual(self.device.latest_sensor_reading_id, reading.id)

    def test_sensor_ignores_older_out_of_order_message(self):
        newer_timestamp = datetime(2030, 1, 1, tzinfo=UTC)
        older_timestamp = datetime(2020, 1, 1, tzinfo=UTC)

        newer = SensorReading.objects.create(
            device=self.device,
            iaq=10,
            co2_equivalent=10,
            voc_equivalent=10,
            temperature=10,
            humidity=10,
            pressure=10,
            accuracy=1,
            timestamp=newer_timestamp,
        )
        self.device.latest_sensor_reading = newer
        self.device.save()

        payload = json.dumps(
            {
                "iaq": 50.0,
                "co2": 400.0,
                "voc": 0.5,
                "temp": 21.5,
                "hum": 45.0,
                "pressure": 1013.0,
                "iaq_accuracy": 3,
                "timestamp": int(older_timestamp.timestamp()),
            }
        )
        self.handler.on_message(
            None,
            None,
            FakeMessage(f"iaq/{self.device.mac}/sensor_data", to_bytes(payload)),
        )

        self.device.refresh_from_db()
        self.assertEqual(self.device.latest_sensor_reading_id, newer.id)

    def test_sensor_invalid_json_is_logged_not_raised(self):
        with self.assertLogs("mqtt.handlers", level="WARNING"):
            self.handler.on_message(
                None,
                None,
                FakeMessage(f"iaq/{self.device.mac}/sensor_data", to_bytes("not json")),
            )
        self.assertFalse(SensorReading.objects.exists())

    def test_sensor_missing_field_is_logged_not_raised(self):
        payload = json.dumps({"iaq": 50.0})
        with self.assertLogs("mqtt.handlers", level="WARNING"):
            self.handler.on_message(
                None,
                None,
                FakeMessage(f"iaq/{self.device.mac}/sensor_data", to_bytes(payload)),
            )
        self.assertFalse(SensorReading.objects.exists())

    def test_health_creates_reading(self):
        payload = json.dumps(
            {
                "rssi": -60,
                "heap": 1000,
                "min_heap": 500,
                "uptime": 12345,
                "timestamp": 1700000000,
            }
        )
        self.handler.on_message(
            None,
            None,
            FakeMessage(f"iaq/{self.device.mac}/device_health", to_bytes(payload)),
        )

        reading = HealthReading.objects.get()
        self.assertEqual(reading.device_id, self.device.id)
        self.assertEqual(reading.rssi, -60)
        self.assertEqual(reading.uptime, 12345)

        self.device.refresh_from_db()
        self.assertEqual(self.device.latest_health_reading_id, reading.id)

    def test_health_ignores_older_out_of_order_message(self):
        newer_timestamp = datetime(2030, 1, 1, tzinfo=UTC)
        older_timestamp = datetime(2020, 1, 1, tzinfo=UTC)

        newer = HealthReading.objects.create(
            device=self.device,
            rssi=-50,
            heap=1000,
            min_heap=500,
            uptime=1,
            timestamp=newer_timestamp,
        )
        self.device.latest_health_reading = newer
        self.device.save()

        payload = json.dumps(
            {
                "rssi": -60,
                "heap": 1000,
                "min_heap": 500,
                "uptime": 12345,
                "timestamp": int(older_timestamp.timestamp()),
            }
        )
        self.handler.on_message(
            None,
            None,
            FakeMessage(f"iaq/{self.device.mac}/device_health", to_bytes(payload)),
        )

        self.device.refresh_from_db()
        self.assertEqual(self.device.latest_health_reading_id, newer.id)

    def test_info_creates_reading(self):
        payload = json.dumps(
            {
                "firmware_version": "1.0.0",
                "chip_model": "ESP32",
                "chip_revision": 1,
                "chip_cores": 2,
                "reset_reason": 1,
                "total_heap": 200000,
                "claim_code": "123456",
            }
        )
        self.handler.on_message(
            None,
            None,
            FakeMessage(f"iaq/{self.device.mac}/device_info", to_bytes(payload)),
        )
        info = DeviceInfo.objects.get(device=self.device)
        self.assertEqual(info.firmware_version, "1.0.0")
        self.assertEqual(info.chip_model, "ESP32")
        self.assertEqual(info.total_heap, 200000)
        self.assertEqual(info.claim_code, "123456")

    def test_status_online(self):
        self.handler.on_message(
            None,
            None,
            FakeMessage(f"iaq/{self.device.mac}/device_status", to_bytes("online")),
        )
        status = DeviceStatus.objects.get(device=self.device)
        self.assertTrue(status.is_online)

    def test_status_unknown_value_is_logged_not_raised(self):
        with self.assertLogs("mqtt.handlers", level="WARNING"):
            self.handler.on_message(
                None,
                None,
                FakeMessage(f"iaq/{self.device.mac}/device_status", to_bytes("maybe")),
            )
        self.assertFalse(DeviceStatus.objects.filter(device=self.device).exists())

    def test_unknown_device_is_logged_not_raised(self):
        with self.assertLogs("mqtt.handlers", level="WARNING"):
            self.handler.on_message(
                None,
                None,
                FakeMessage("iaq/00:00:00:00:00:00/status", to_bytes("online")),
            )
        self.assertEqual(DeviceStatus.objects.count(), 0)

    def test_malformed_topic_is_logged_not_raised(self):
        with self.assertLogs("mqtt.handlers", level="WARNING"):
            self.handler.on_message(
                None, None, FakeMessage("not-a-topic", to_bytes("online"))
            )
