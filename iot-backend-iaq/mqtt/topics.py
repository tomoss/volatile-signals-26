"""MQTT topics consumed by the IAQ ingestion service."""

from typing import NamedTuple


class Subscription(NamedTuple):
    topic: str
    qos: int


PREFIX = "iaq/+/"
SENSOR_TOPIC_NAME = "sensor"
HEALTH_TOPIC_NAME = "health"
INFO_TOPIC_NAME = "info"
STATUS_TOPIC_NAME = "status"

SENSOR = PREFIX + SENSOR_TOPIC_NAME
HEALTH = PREFIX + HEALTH_TOPIC_NAME
INFO = PREFIX + INFO_TOPIC_NAME
STATUS = PREFIX + STATUS_TOPIC_NAME

QOS_AT_LEAST_ONCE = 1

SUBSCRIPTIONS = [
    Subscription(SENSOR, QOS_AT_LEAST_ONCE),
    Subscription(HEALTH, QOS_AT_LEAST_ONCE),
    Subscription(INFO, QOS_AT_LEAST_ONCE),
    Subscription(STATUS, QOS_AT_LEAST_ONCE),
]
