from django.contrib.auth.views import logout_then_login
from django.urls import path

from . import views

urlpatterns = [
    path("account/login/", views.IaqLoginView.as_view(), name="login"),
    path("account/register/", views.IaqRegisterView.as_view(), name="register"),
    path("account/devices/", views.IaqDeviceListView.as_view(), name="devices"),
    path("account/profile/", views.IaqProfileView.as_view(), name="profile"),
    path(
        "account/profile/password/",
        views.IaqPasswordChangeView.as_view(),
        name="password_change",
    ),
    path(
        "account/profile/password/done/",
        views.IaqPasswordChangeDoneView.as_view(),
        name="password_change_done",
    ),
    path(
        "device/<int:device_id>/dashboard/",
        views.IaqDeviceDashboardView.as_view(),
        name="dashboard",
    ),
    path(
        "device/<int:device_id>/history/",
        views.IaqDeviceHistoryView.as_view(),
        name="history",
    ),
    path("logout/", logout_then_login, name="logout"),
    path("", views.IaqHomeView.as_view(), name="home"),
]
