from django.core.management.base import BaseCommand, CommandError

from mqtt.client import MqttClient


class Command(BaseCommand):
    help = "Connects to the MQTT broker and ingests device messages."

    def handle(self, *args, **options):
        try:
            MqttClient().run()
        except ValueError as exc:
            raise CommandError(f"Invalid MQTT connection settings: {exc}") from exc
