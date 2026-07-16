from django.contrib.auth.forms import UserCreationForm
from django import forms
from iaq.models import IaqUser

class IaqUserCreationForm(UserCreationForm):
    email = forms.EmailField(
        error_messages={
            "unique": "An account with this email already exists.",
        },
    )

    class Meta(UserCreationForm.Meta):
        model = IaqUser
        fields = ("email", "first_name", "last_name")
