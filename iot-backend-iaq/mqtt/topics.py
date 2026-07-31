"""MQTT topics consumed by the IAQ ingestion service."""

from collections import namedtuple

Subscription = namedtuple("Subscription", ["topic", "qos"])

PREFIX = "iaq/+/"
SENSOR_TOPIC_SUFFIX = "sensor"
HEALTH_TOPIC_SUFFIX = "health"
INFO_TOPIC_SUFFIX = "info"
STATUS_TOPIC_SUFFIX = "status"

SENSOR_TOPIC = f"{PREFIX}{SENSOR_TOPIC_SUFFIX}"
HEALTH_TOPIC = f"{PREFIX}{HEALTH_TOPIC_SUFFIX}"
INFO_TOPIC = f"{PREFIX}{INFO_TOPIC_SUFFIX}"
STATUS_TOPIC = f"{PREFIX}{STATUS_TOPIC_SUFFIX}"

QOS_LEVEL = 1

SUBSCRIPTIONS = [
    Subscription(SENSOR_TOPIC, QOS_LEVEL),
    Subscription(HEALTH_TOPIC, QOS_LEVEL),
    Subscription(INFO_TOPIC, QOS_LEVEL),
    Subscription(STATUS_TOPIC, QOS_LEVEL),
]
