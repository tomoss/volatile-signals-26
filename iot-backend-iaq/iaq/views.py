from django.urls import reverse_lazy
from django.views.generic import CreateView, ListView, TemplateView
from django.contrib.auth.views import LoginView
from django.contrib.auth.mixins import LoginRequiredMixin

from iaq.models import Device, SensorReading
from .forms import IaqUserCreationForm

from django.db.models import OuterRef, Subquery

class IaqHomeView(ListView):
    template_name = "iaq/home.html"
    model = Device
    context_object_name = "device_list"

    def get_queryset(self):
        latest = SensorReading.objects.filter(device=OuterRef("pk")).order_by("-timestamp")

        return Device.objects.filter(is_public=True).annotate(
            latest_iaq=Subquery(
                latest.values("iaq")[:1]
            ),
            latest_timestamp=Subquery(
                latest.values("timestamp")[:1]
            ),
            latest_temperature=Subquery(
                latest.values("temperature")[:1]
            ),
            latest_humidity=Subquery(
                latest.values("humidity")[:1]
            ),
            latest_pressure=Subquery(
                latest.values("pressure")[:1]
            ),
        )

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
        latest = SensorReading.objects.filter(device=OuterRef("pk")).order_by("-timestamp")

        return Device.objects.filter(user=self.request.user).annotate(
            latest_iaq=Subquery(
                latest.values("iaq")[:1]
            ),
            latest_timestamp=Subquery(
                latest.values("timestamp")[:1]
            ),
            latest_temperature=Subquery(
                latest.values("temperature")[:1]
            ),
            latest_humidity=Subquery(
                latest.values("humidity")[:1]
            ),
            latest_pressure=Subquery(
                latest.values("pressure")[:1]
            ),
        )

class IaqProfileView(LoginRequiredMixin, TemplateView):
    template_name = "iaq/profile.html"