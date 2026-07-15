from django.shortcuts import render
from django.views.generic import ListView, TemplateView
from django.contrib.auth.views import LoginView
from django.contrib.auth.mixins import LoginRequiredMixin

from iaq.models import Device

class IaqHomeView(ListView):
    template_name = "iaq/home.html"
    model = Device

    def get_queryset(self):
        return Device.objects.filter(is_public=True)

class IaqLoginView(LoginView):
    template_name = "iaq/login.html"
    redirect_authenticated_user = True

class IaqAccountView(LoginRequiredMixin, TemplateView):
    template_name = "iaq/account.html"



