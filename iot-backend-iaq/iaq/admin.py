from django.contrib import admin

from .models import (
    Device,
    DeviceInfo,
    DeviceStatus,
    HealthReading,
    IaqUser,
    SensorInfo,
    SensorReading,
)


class IaqUserAdmin(admin.ModelAdmin):
    # Plain ModelAdmin instead of auth.UserAdmin: UserAdmin's fieldsets/forms
    # hardcode "username"/"date_joined", which IaqUser doesn't have. Password
    # is excluded because an editable text field would show the raw hash and
    # save whatever gets typed in without hashing it, breaking login.
    exclude = ("password",)
    list_display = ("email", "first_name", "last_name", "is_staff", "is_active")


admin.site.register(IaqUser, IaqUserAdmin)
admin.site.register(Device)
admin.site.register(SensorReading)
admin.site.register(HealthReading)
admin.site.register(DeviceInfo)
admin.site.register(DeviceStatus)
admin.site.register(SensorInfo)
