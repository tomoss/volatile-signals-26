from django.contrib.auth.models import AbstractBaseUser, PermissionsMixin
from django.db import models

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
    latest_sensor_reading = models.ForeignKey(
        "SensorReading",
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name="+",
    )
    latest_health_reading = models.ForeignKey(
        "HealthReading",
        null=True,
        blank=True,
        on_delete=models.SET_NULL,
        related_name="+",
    )


class SensorReading(models.Model):
    device = models.ForeignKey(
        Device, on_delete=models.CASCADE, related_name="sensor_readings"
    )
    iaq = models.FloatField()
    co2_equivalent = models.FloatField()
    voc_equivalent = models.FloatField()
    temperature = models.FloatField()
    humidity = models.FloatField()
    pressure = models.FloatField()
    accuracy = models.PositiveBigIntegerField()
    timestamp = models.DateTimeField()


class HealthReading(models.Model):
    device = models.ForeignKey(
        Device, on_delete=models.CASCADE, related_name="device_health"
    )
    rssi = models.SmallIntegerField()
    heap = models.PositiveIntegerField()
    min_heap = models.PositiveIntegerField()
    uptime = models.PositiveIntegerField()
    timestamp = models.DateTimeField()
    received_at = models.DateTimeField(auto_now_add=True)


class DeviceInfo(models.Model):
    device = models.OneToOneField(
        Device, on_delete=models.CASCADE, related_name="device_info"
    )
    firmware_version = models.CharField(max_length=40, default="")
    chip_model = models.CharField(max_length=40, default="")
    chip_revision = models.PositiveSmallIntegerField(default=0)
    chip_cores = models.PositiveSmallIntegerField(default=0)
    reset_reason = models.SmallIntegerField(default=0)
    total_heap = models.PositiveIntegerField(default=0)
    sensor_mode = models.PositiveSmallIntegerField(default=0)
    claim_code = models.CharField(max_length=6, default="")


class DeviceStatus(models.Model):
    device = models.OneToOneField(
        Device, on_delete=models.CASCADE, related_name="device_status"
    )
    is_online = models.BooleanField(default=False)
    received_at = models.DateTimeField(auto_now=True)
