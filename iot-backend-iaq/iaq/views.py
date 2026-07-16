from django.urls import reverse_lazy
from django.views.generic import CreateView, ListView, TemplateView
from django.contrib.auth.views import LoginView
from django.contrib.auth.mixins import LoginRequiredMixin

from iaq.models import Device, SensorReading
from .forms import IaqUserCreationForm

class IaqHomeView(ListView):
    template_name = "iaq/home.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        return (Device.objects.filter(is_public=True).select_related("latest_reading"))

class IaqLoginView(LoginView):
    template_name = "iaq/login.html"
    redirect_authenticated_user = True

# TODO: Populate Account page with info, etc
class IaqAccountView(LoginRequiredMixin, TemplateView):
    template_name = "iaq/account.html"

class IaqRegisterView(CreateView):
    template_name = "iaq/register.html"
    form_class = IaqUserCreationForm
    success_url = reverse_lazy("login")

class IaqDeviceDashboardView(TemplateView):
    template_name = "iaq/dashboard.html"

class IaqDeviceHistoryView(TemplateView):
    template_name = "iaq/history.html"

class IaqDevicesView(LoginRequiredMixin, ListView):
    template_name = "iaq/devices.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        return (Device.objects.filter(user=self.request.user).select_related("latest_reading"))

class IaqProfileView(LoginRequiredMixin, TemplateView):
    template_name = "iaq/profile.html"