from datetime import datetime, time

from django.contrib import messages
from django.contrib.auth.mixins import LoginRequiredMixin
from django.contrib.auth.views import (
    LoginView,
    PasswordChangeDoneView,
    PasswordChangeView,
)
from django.db import IntegrityError, transaction
from django.db.models import Q
from django.shortcuts import redirect
from django.urls import reverse_lazy
from django.utils import timezone
from django.views import View
from django.views.generic import (
    CreateView,
    DetailView,
    FormView,
    ListView,
    TemplateView,
)

from iaq.models import Device, DeviceClaim, DeviceStatus
from mqtt.publisher import publish_command

from .forms import (
    DeviceClaimForm,
    HistoryFilterForm,
    IaqUserCreationForm,
    default_date_range,
)


class IaqHomeView(ListView):
    template_name = "iaq/home.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        return Device.objects.filter(is_public=True).select_related(
            "latest_sensor_reading", "device_info"
        )


class IaqLoginView(LoginView):
    template_name = "iaq/login.html"
    redirect_authenticated_user = True


class IaqRegisterView(CreateView):
    template_name = "iaq/register.html"
    form_class = IaqUserCreationForm
    success_url = reverse_lazy("login")


class IaqDeviceDashboardView(DetailView):
    template_name = "iaq/dashboard.html"
    model = Device
    pk_url_kwarg = "device_id"
    context_object_name = "device"

    def get_queryset(self):
        qs = Device.objects.select_related("latest_sensor_reading")
        if self.request.user.is_authenticated:
            return qs.filter(Q(is_public=True) | Q(user=self.request.user))
        return qs.filter(is_public=True)

    def get_template_names(self):
        if self.request.headers.get("HX-Request") == "true":
            return ["iaq/dashboard_sensor_reading.html"]
        return [self.template_name]


class IaqDeviceHistoryView(DetailView):
    template_name = "iaq/history.html"
    model = Device
    pk_url_kwarg = "device_id"
    context_object_name = "device"

    def get_queryset(self):
        if self.request.user.is_authenticated:
            return Device.objects.filter(Q(is_public=True) | Q(user=self.request.user))
        return Device.objects.filter(is_public=True)

    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)

        default_dates = default_date_range()

        start_date = (
            self.request.GET.get("start_date") or default_dates.start.isoformat()
        )

        end_date = self.request.GET.get("end_date") or default_dates.end.isoformat()

        data = {"start_date": start_date, "end_date": end_date}

        form = HistoryFilterForm(data)
        context["form"] = form

        if not form.is_valid():
            context["sensor_readings"] = []
            return context

        start_date = form.cleaned_data["start_date"]
        end_date = form.cleaned_data["end_date"]

        start_dt = timezone.make_aware(datetime.combine(start_date, time.min))
        end_dt = timezone.make_aware(datetime.combine(end_date, time.max))

        readings = (
            self.object.sensor_readings.filter(
                timestamp__range=(start_dt, end_dt), accuracy=3
            )
            .order_by("timestamp")
            .values(
                "timestamp",
                "iaq",
                "co2_equivalent",
                "voc_equivalent",
                "temperature",
                "humidity",
                "pressure",
            )
        )

        context["sensor_readings"] = list(readings)
        return context


class IaqDeviceListView(LoginRequiredMixin, ListView):
    template_name = "iaq/devices.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        return Device.objects.filter(user=self.request.user).select_related(
            "latest_sensor_reading", "device_info"
        )


class IaqDeviceAddView(LoginRequiredMixin, FormView):
    template_name = "iaq/add_device.html"
    form_class = DeviceClaimForm
    success_url = reverse_lazy("devices")

    def form_valid(self, form):
        cleaned = form.cleaned_data

        try:
            claim = DeviceClaim.objects.get(claim_code=cleaned["claim_code"])
        except DeviceClaim.DoesNotExist:
            form.add_error("claim_code", "No device found with this claim code.")
            return self.form_invalid(form)

        try:
            with transaction.atomic():
                device = Device.objects.create(
                    user=self.request.user,
                    mac=claim.mac,
                    name=cleaned["name"],
                    is_public=cleaned["is_public"],
                )
                claim.delete()
        except IntegrityError:
            # Device.mac is unique; another user claimed it first.
            form.add_error(None, "This device has already been claimed.")
            return self.form_invalid(form)

        if publish_command(device.mac, "device_claimed"):
            messages.success(self.request, f"Device '{device.name}' added.")
        else:
            messages.warning(
                self.request,
                f"Device '{device.name}' added, but couldn't reach device to "
                "confirm the claim.",
            )

        return super().form_valid(form)


class IaqProfileView(LoginRequiredMixin, TemplateView):
    template_name = "iaq/profile.html"


class IaqPasswordChangeView(LoginRequiredMixin, PasswordChangeView):
    template_name = "iaq/password_change.html"
    success_url = reverse_lazy("password_change_done")


class IaqPasswordChangeDoneView(LoginRequiredMixin, PasswordChangeDoneView):
    template_name = "iaq/password_change_done.html"


class IaqDeviceManagementView(LoginRequiredMixin, DetailView):
    template_name = "iaq/device_management.html"
    model = Device
    pk_url_kwarg = "device_id"
    context_object_name = "device"

    def get_queryset(self):
        return Device.objects.select_related(
            "latest_health_reading", "device_info", "device_status"
        )


class IaqDeviceCommandView(LoginRequiredMixin, View):
    command = None
    success_message = None
    error_message = None

    def get(self, request, *args, **kwargs):
        device_id = self.kwargs.get("device_id")
        device = Device.objects.get(pk=device_id)

        try:
            is_online = device.device_status.is_online
        except DeviceStatus.DoesNotExist:
            is_online = False

        if not is_online:
            messages.error(request, "Device is offline.")
            return redirect("device_management", device_id=device_id)

        if publish_command(device.mac, self.command):
            messages.success(request, self.success_message)
        else:
            messages.error(request, self.error_message)

        return redirect("device_management", device_id=device_id)


class IaqDeviceToggleVisibilityView(LoginRequiredMixin, View):
    def get(self, request, *args, **kwargs):
        device_id = self.kwargs.get("device_id")
        device = Device.objects.get(pk=device_id)
        device.is_public = not device.is_public
        device.save(update_fields=["is_public"])

        if device.is_public:
            messages.success(request, "Device is now public.")
        else:
            messages.success(request, "Device is now private.")

        return redirect("device_management", device_id=device_id)


class IaqDeviceRebootView(IaqDeviceCommandView):
    command = "reboot"
    success_message = "Reboot command sent."
    error_message = "Failed to send reboot command; broker unreachable."


class IaqSensorLowPowerView(IaqDeviceCommandView):
    command = "sensor_lp"
    success_message = "Low power command sent."
    error_message = "Failed to send low power command; broker unreachable."


class IaqSensorUltraLowPowerView(IaqDeviceCommandView):
    command = "sensor_ulp"
    success_message = "Ultra low power command sent."
    error_message = "Failed to send ultra low power command; broker unreachable."
