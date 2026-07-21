from django.urls import reverse_lazy
from django.views.generic import CreateView, ListView, TemplateView
from django.contrib.auth.views import LoginView, PasswordChangeView, PasswordChangeDoneView
from django.contrib.auth.mixins import LoginRequiredMixin

from iaq.models import Device, SensorReading
from .forms import IaqUserCreationForm

class IaqHomeView(ListView):
    template_name = "iaq/home.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        return Device.objects.filter(is_public=True).select_related("latest_reading", "device_info")

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
        return Device.objects.filter(is_public=True).select_related("latest_reading", "device_info")

class IaqProfileView(LoginRequiredMixin, TemplateView):
    template_name = "iaq/profile.html"

class IaqPasswordChangeView(LoginRequiredMixin, PasswordChangeView):
    template_name = "iaq/password_change.html"
    success_url = reverse_lazy("password_change_done")

class IaqPasswordChangeDoneView(LoginRequiredMixin, PasswordChangeDoneView):
    template_name = "iaq/password_change_done.html"