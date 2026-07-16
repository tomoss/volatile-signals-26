from django.db import models
from django.contrib.auth.models import AbstractBaseUser, PermissionsMixin

from iaq.managers import IaqUserManager

class IaqUser(AbstractBaseUser, PermissionsMixin):
    email = models.EmailField(unique=True)
    first_name = models.CharField(max_length=40, default="")
    last_name = models.CharField(max_length=40, default="")
    is_staff = models.BooleanField(default=False)
    is_active = models.BooleanField(default=True)

    USERNAME_FIELD = "email"
    REQUIRED_FIELDS = ["first_name", "last_name"]
    objects = IaqUserManager()

class Device(models.Model):
    user = models.ForeignKey(IaqUser, on_delete=models.CASCADE, related_name="devices")
    mac = models.CharField(max_length=17, unique=True)
    name = models.CharField(max_length=100, default="")
    is_public = models.BooleanField(default=False)
    latest_reading = models.ForeignKey("SensorReading", null=True, blank=True, on_delete=models.SET_NULL, related_name="+")

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
