"""Short-lived MQTT publisher for sending commands from the Django web process."""

import logging

import paho.mqtt.client as mqtt
from django.conf import settings

from mqtt.topics import QOS_LEVEL, create_command_topic, create_ota_topic

logger = logging.getLogger(__name__)

PUBLISH_TIMEOUT = 0.5  # 500 milliseconds


def _publish(topic: str, payload: str) -> bool:
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        protocol=mqtt.MQTTv5,
    )
    if settings.MQTT_USERNAME:
        client.username_pw_set(settings.MQTT_USERNAME, settings.MQTT_PASSWORD)
    if settings.MQTT_TLS:
        client.tls_set()

    try:
        client.connect(settings.MQTT_HOST, settings.MQTT_PORT)
        client.loop_start()
        info = client.publish(topic, payload, qos=QOS_LEVEL)
        info.wait_for_publish(timeout=PUBLISH_TIMEOUT)
    except (OSError, ValueError) as exc:
        logger.warning("Failed to publish %r to %s: %s", payload, topic, exc)
        return False
    finally:
        client.loop_stop()
        client.disconnect()

    return True


def publish_command(mac: str, command: str) -> bool:
    return _publish(create_command_topic(mac), command)


def publish_ota(mac: str, url: str) -> bool:
    """Trigger a device OTA update.

    The firmware listens on a separate `.../ota` topic (not the regular
    command topic) and expects the raw download URL as the payload, over
    plain HTTP -- it connects with a non-TLS WiFiClient.
    """
    return _publish(create_ota_topic(mac), url)
