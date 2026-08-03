from django.contrib import messages
from django.contrib.auth.mixins import LoginRequiredMixin
from django.contrib.auth.views import (
    LoginView,
    PasswordChangeDoneView,
    PasswordChangeView,
)
from django.shortcuts import redirect
from django.urls import reverse_lazy
from django.views import View
from django.views.generic import CreateView, DetailView, ListView, TemplateView

from iaq.models import Device
from mqtt.publisher import publish_command

from .forms import IaqUserCreationForm


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
        return Device.objects.filter(is_public=True).select_related(
            "latest_sensor_reading", "device_info"
        )


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
            "latest_health_reading", "device_info", "device_status", "sensor_info"
        )


class IaqDeviceRebootView(LoginRequiredMixin, View):
    def get(self, request, *args, **kwargs):
        device_id = self.kwargs.get("device_id")
        device = Device.objects.get(pk=device_id)

        try:
            publish_command(device.mac, "reboot")
        except (OSError, ValueError):
            messages.error(
                request, "Failed to send reboot command; broker unreachable."
            )
        else:
            messages.success(request, "Reboot command sent.")

        return redirect("device_management", device_id=device_id)
