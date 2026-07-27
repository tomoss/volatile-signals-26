"""Application-level handling for messages received from IAQ devices."""


class MessageHandler:
    """Validate and persist messages received on IAQ MQTT topics."""

    def __init__(self):
        pass

    def handle(self, topic: str, payload: bytes) -> None:
        pass
