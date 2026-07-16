from django.contrib.auth.views import logout_then_login
from django.urls import path

from . import views

urlpatterns = [
    path("account/login/", views.IaqLoginView.as_view(), name="login"),
    path("account/register/", views.IaqRegisterView.as_view(), name="register"),
    path("account/", views.IaqAccountView.as_view(), name="account"),
    path("device/<int:device_id>/dashboard/", views.IaqDeviceDashboardView.as_view(), name="dashboard"),
    path("device/<int:device_id>/history/", views.IaqDeviceHistoryView.as_view(), name="history"),
    path("logout/", logout_then_login, name="logout"),
    path("", views.IaqHomeView.as_view(), name="home"),
]