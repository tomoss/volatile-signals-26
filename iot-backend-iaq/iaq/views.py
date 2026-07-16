from django.urls import reverse_lazy
from django.views.generic import CreateView, ListView, TemplateView
from django.contrib.auth.views import LoginView
from django.contrib.auth.mixins import LoginRequiredMixin

from iaq.models import Device
from .forms import IaqUserCreationForm

class IaqHomeView(ListView):
    template_name = "iaq/home.html"
    model = Device

    def get_queryset(self):
        return Device.objects.filter(is_public=True)

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


