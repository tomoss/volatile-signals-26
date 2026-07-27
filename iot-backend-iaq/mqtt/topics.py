"""MQTT topics consumed by the IAQ ingestion service."""

from typing import NamedTuple


class Subscription(NamedTuple):
    topic: str
    qos: int


SENSOR = "iaq/+/sensor"
HEALTH = "iaq/+/health"
INFO = "iaq/+/info"
STATUS = "iaq/+/status"

QOS_AT_LEAST_ONCE = 1

SUBSCRIPTIONS = [
    Subscription(SENSOR, QOS_AT_LEAST_ONCE),
    Subscription(HEALTH, QOS_AT_LEAST_ONCE),
    Subscription(INFO, QOS_AT_LEAST_ONCE),
    Subscription(STATUS, QOS_AT_LEAST_ONCE),
]
