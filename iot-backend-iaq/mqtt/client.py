"""MQTT broker lifecycle management."""

import logging
import signal
import time

import paho.mqtt.client as mqtt
from django.conf import settings
from django.db import close_old_connections

from mqtt.handlers import MessageHandler
from mqtt.topics import SUBSCRIPTIONS

logger = logging.getLogger(__name__)


class MqttClient:
    """Connect to the broker and delegate incoming messages to a handler."""

    def __init__(self, message_handler: MessageHandler | None = None):
        self._handler = message_handler or MessageHandler()
        self._client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2
        )
        self._configure_client()

    def _configure_client(self) -> None:
        if settings.MQTT_USERNAME:
            self._client.username_pw_set(settings.MQTT_USERNAME, settings.MQTT_PASSWORD)
        if settings.MQTT_TLS:
            self._client.tls_set()

        self._client.reconnect_delay_set(min_delay=1, max_delay=60)
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

    def run(self) -> None:
        self._register_signal_handlers()
        self._connect_with_backoff()
        self._client.loop_forever()

    def _connect_with_backoff(self) -> None:
        delay = 1
        while True:
            try:
                self._client.connect(settings.MQTT_HOST, settings.MQTT_PORT)
                return
            except OSError as exc:
                logger.warning(
                    "MQTT connection to %s:%s failed (%s); retrying in %ss",
                    settings.MQTT_HOST,
                    settings.MQTT_PORT,
                    exc,
                    delay,
                )
                time.sleep(delay)
                delay = min(delay * 2, 60)

    def _register_signal_handlers(self) -> None:
        try:
            signal.signal(signal.SIGINT, self._shutdown)
            signal.signal(signal.SIGTERM, self._shutdown)
        except ValueError:
            # Signals can only be registered from the process main thread.
            logger.warning("MQTT signal handlers were not registered")

    def _shutdown(self, signum, frame) -> None:
        logger.info("Received signal %s; disconnecting MQTT client", signum)
        self._client.disconnect()

    @staticmethod
    def _on_connect(client, userdata, flags, reason_code, properties=None) -> None:
        if reason_code != 0:
            logger.error("MQTT connection failed: %s", reason_code)
            return

        client.subscribe(SUBSCRIPTIONS)
        logger.info("Connected to MQTT broker and subscribed to IAQ topics")

    @staticmethod
    def _on_disconnect(
        client, userdata, disconnect_flags, reason_code, properties=None
    ) -> None:
        logger.warning("Disconnected from MQTT broker: %s", reason_code)

    def _on_message(self, client, userdata, message) -> None:
        # Runs on paho's loop thread, outside Django's request cycle, so stale/
        # dropped DB connections aren't closed automatically as they would be
        # via the request_started/request_finished signals.
        close_old_connections()
        try:
            self._handler.handle(message.topic, message.payload)
        except Exception:
            logger.exception(
                "Failed to process MQTT message on topic %s", message.topic
            )
        finally:
            close_old_connections()
