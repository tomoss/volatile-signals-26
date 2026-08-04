"""Short-lived MQTT publisher for sending commands from the Django web process."""

import logging

import paho.mqtt.client as mqtt
from django.conf import settings

from mqtt.topics import QOS_LEVEL, create_command_topic

logger = logging.getLogger(__name__)

PUBLISH_TIMEOUT = 0.5  # 500 milliseconds


def publish_command(mac: str, command: str) -> bool:
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
        info = client.publish(create_command_topic(mac), command, qos=QOS_LEVEL)
        info.wait_for_publish(timeout=PUBLISH_TIMEOUT)
    except (OSError, ValueError) as exc:
        logger.warning("Failed to publish command %r to %s: %s", command, mac, exc)
        return False
    finally:
        client.loop_stop()
        client.disconnect()

    return True
