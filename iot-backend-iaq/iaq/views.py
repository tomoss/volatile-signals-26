from django.shortcuts import render
from django.views.generic import ListView

from iaq.models import Device

class HomeView(ListView):
    template_name = "iaq/home.html"
    model = Device

    def get_queryset(self):
        return Device.objects.filter(is_public=True)


