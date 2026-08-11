from django import forms
from django.contrib.auth.forms import UserCreationForm
from django.core.validators import RegexValidator

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


class DeviceClaimForm(forms.Form):
    name = forms.CharField(
        label="Device name",
        max_length=100,
        widget=forms.TextInput(
            attrs={
                "placeholder": "e.g. Living Room Sensor",
                "autocomplete": "off",
            }
        ),
    )

    claim_code = forms.CharField(
        label="Claim code",
        min_length=6,
        max_length=6,
        validators=[
            RegexValidator(
                regex=r"^\d{6}$",
                message="Claim code must be exactly 6 digits.",
            )
        ],
        widget=forms.TextInput(
            attrs={
                "placeholder": "123456",
                "inputmode": "numeric",
                "pattern": r"\d{6}",
                "maxlength": 6,
                "autocomplete": "one-time-code",
            }
        ),
    )

    is_public = forms.BooleanField(
        label="Make this device public",
        required=False,
        help_text="Public devices are visible to anyone on the home page.",
    )
