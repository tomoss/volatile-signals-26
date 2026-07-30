import logging
import signal
import time

import paho.mqtt.client as mqtt
from django.conf import settings
from django.core.management.base import BaseCommand, CommandError

from mqtt.handlers import MessageHandler
from mqtt.topics import SUBSCRIPTIONS

logger = logging.getLogger(__name__)


class Command(BaseCommand):
    help = "Connects to the MQTT broker and ingests device messages."

    def handle(self, *args, **options):
        try:
            self._handler = MessageHandler()
            self._client = mqtt.Client(
                callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
                protocol=mqtt.MQTTv5,
            )

            if settings.MQTT_USERNAME:
                self._client.username_pw_set(
                    settings.MQTT_USERNAME, settings.MQTT_PASSWORD
                )
            if settings.MQTT_TLS:
                self._client.tls_set()
            self._client.reconnect_delay_set(min_delay=1, max_delay=60)

            self._client.on_connect = self._on_connect
            self._client.on_disconnect = self._on_disconnect
            self._client.on_subscribe = self._on_subscribe
            self._client.on_message = self._handler.handle

            signal.signal(signal.SIGINT, self._shutdown)
            signal.signal(signal.SIGTERM, self._shutdown)

            self._connect_with_backoff()
            self._client.loop_forever()

        except Exception as exc:
            raise CommandError(f"MQTT client failed: {exc}") from exc

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

    def _shutdown(self, signum, frame) -> None:
        logger.info("Received signal %s; disconnecting MQTT client", signum)
        self._client.disconnect()

    @staticmethod
    def _on_connect(client, userdata, flags, reason_code, properties) -> None:
        if reason_code.is_failure:
            logger.error("MQTT connection failed: %s", reason_code)
            return

        client.subscribe(SUBSCRIPTIONS)
        logger.info("Connected to MQTT broker and subscribed to IAQ topics")

    @staticmethod
    def _on_disconnect(
        client, userdata, disconnect_flags, reason_code, properties
    ) -> None:
        if reason_code.is_failure:
            logger.error("MQTT disconnection error: %s", reason_code)
        else:
            logger.warning("Disconnected from MQTT broker: %s", reason_code)

    @staticmethod
    def _on_subscribe(client, userdata, mid, reason_code_list, properties):
        for subscription, reason_code in zip(SUBSCRIPTIONS, reason_code_list):
            if reason_code.is_failure:
                logger.error(
                    "Subscription to %s failed: %s", subscription.topic, reason_code
                )
            else:
                logger.info(
                    "Subscribed to %s with QoS %s",
                    subscription.topic,
                    reason_code.value,
                )
