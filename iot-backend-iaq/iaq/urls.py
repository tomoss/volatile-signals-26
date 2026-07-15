from django.contrib.auth.views import logout_then_login
from django.urls import path

from . import views

urlpatterns = [
    path("", views.IaqHomeView.as_view(), name="home"),
    path("account/login/", views.IaqLoginView.as_view(), name="login"),
    path("logout", logout_then_login, name="logout"),
    path("account", views.IaqAccountView.as_view(), name="account")
]