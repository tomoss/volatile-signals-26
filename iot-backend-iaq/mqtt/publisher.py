"""Short-lived MQTT publisher for sending commands from the Django web process."""

import json
import logging

import paho.mqtt.client as mqtt
from django.conf import settings

from mqtt.topics import QOS_LEVEL, create_topic

logger = logging.getLogger(__name__)

PUBLISH_TIMEOUT = 0.5  # 500 milliseconds


def publish(mac: str, topic_suffix: str, payload: dict) -> bool:
    topic = create_topic(mac, topic_suffix)
    message = json.dumps(payload)

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
        info = client.publish(topic, message, qos=QOS_LEVEL)
        info.wait_for_publish(timeout=PUBLISH_TIMEOUT)
    except (OSError, ValueError) as exc:
        logger.warning("Failed to publish %s to %s: %s", message, topic, exc)
        return False
    finally:
        client.loop_stop()
        client.disconnect()

    return True
