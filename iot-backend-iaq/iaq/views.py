from django.contrib import messages
from django.contrib.auth.mixins import LoginRequiredMixin
from django.contrib.auth.views import (
    LoginView,
    PasswordChangeDoneView,
    PasswordChangeView,
)
from django.db import IntegrityError, transaction
from django.shortcuts import redirect, render
from django.urls import reverse_lazy
from django.views import View
from django.views.generic import CreateView, DetailView, ListView, TemplateView

from iaq.models import Device, DeviceClaim
from mqtt.publisher import publish_command

from .forms import DeviceClaimForm, IaqUserCreationForm


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


class IaqDeviceDashboardView(TemplateView):
    template_name = "iaq/dashboard.html"


class IaqDeviceHistoryView(TemplateView):
    template_name = "iaq/history.html"


class IaqDeviceListView(LoginRequiredMixin, ListView):
    template_name = "iaq/devices.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        return Device.objects.filter(user=self.request.user).select_related(
            "latest_sensor_reading", "device_info"
        )


class IaqDeviceAddView(LoginRequiredMixin, View):
    template_name = "iaq/add_device.html"

    def get(self, request, *args, **kwargs):
        return render(request, self.template_name, {"form": DeviceClaimForm()})

    def post(self, request, *args, **kwargs):
        form = DeviceClaimForm(request.POST)
        if not form.is_valid():
            return render(request, self.template_name, {"form": form})

        claim_code = form.cleaned_data["claim_code"]
        try:
            claim = DeviceClaim.objects.get(claim_code=claim_code)
        except DeviceClaim.DoesNotExist:
            form.add_error("claim_code", "No device found with this claim code.")
            return render(request, self.template_name, {"form": form})

        cleaned = form.cleaned_data

        def finalize_claim():
            if publish_command(device.mac, "device_claimed"):
                messages.success(request, f"Device '{device.name}' added.")
            else:
                messages.warning(
                    request,
                    f"Device '{device.name}' added, but couldn't reach device to "
                    "confirm the claim.",
                )

        try:
            with transaction.atomic():
                device = Device.objects.create(
                    user=request.user,
                    mac=claim.mac,
                    name=cleaned["name"],
                    is_public=cleaned["is_public"],
                )
                claim.delete()
                transaction.on_commit(finalize_claim)
        except IntegrityError:
            # Device.mac is unique; another user claimed it first.
            form.add_error(None, "This device has already been claimed.")
            return render(request, self.template_name, {"form": form})

        return redirect("devices")


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

        if not device.device_status.is_online:
            messages.error(request, "Device is offline.")
            return redirect("device_management", device_id=device_id)

        if publish_command(device.mac, self.command):
            messages.success(request, self.success_message)
        else:
            messages.error(request, self.error_message)

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
