from django.db import models

class Device(models.Model):
    mac = models.CharField(max_length=17, unique=True)
    name = models.CharField(max_length=100, default="")
    is_public = models.BooleanField(default=False)

class SensorReading(models.Model):
    device = models.ForeignKey(Device, on_delete=models.CASCADE, related_name="sensor_readings")
    iaq = models.FloatField()
    co2_equivalent = models.FloatField()
    voc_equivalent = models.FloatField()
    temperature = models.FloatField()
    humidity = models.FloatField()
    pressure = models.FloatField()
    accuracy = models.PositiveBigIntegerField()
    timestamp = models.DateTimeField()

class HealthReading(models.Model):
    device = models.ForeignKey(Device, on_delete=models.CASCADE, related_name="health_readings")
    rssi = models.SmallIntegerField()
    uptime = models.PositiveIntegerField()
    heap = models.PositiveIntegerField()
    min_heap = models.PositiveIntegerField()
    firmware_version = models.CharField(max_length=40)
    reset_reason = models.SmallIntegerField(default=0)
    received_at = models.DateTimeField(auto_now_add=True)
