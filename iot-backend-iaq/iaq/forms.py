from collections import namedtuple
from datetime import timedelta

from django import forms
from django.contrib.auth.forms import UserCreationForm
from django.core.validators import RegexValidator
from django.utils import timezone

from iaq.models import IaqUser

DEFAULT_HISTORY_RANGE_DAYS = 7

DateRange = namedtuple("DateRange", ["start", "end"])


def default_date_range():
    today = timezone.localdate()
    return DateRange(today - timedelta(days=DEFAULT_HISTORY_RANGE_DAYS), today)


class IaqUserCreationForm(UserCreationForm):
    email = forms.EmailField(
        error_messages={
            "unique": "An account with this email already exists.",
        },
    )

    class Meta(UserCreationForm.Meta):
        model = IaqUser
        fields = ("email", "first_name", "last_name")


class HistoryFilterForm(forms.Form):
    max_range_days = 365

    start_date = forms.DateField(
        required=False,
        widget=forms.DateInput(
            attrs={"type": "date", "class": "form-control", "id": "start_date"}
        ),
    )
    end_date = forms.DateField(
        required=False,
        widget=forms.DateInput(
            attrs={"type": "date", "class": "form-control", "id": "end_date"}
        ),
    )

    def clean(self):
        cleaned_data = super().clean()

        default_dates = default_date_range()
        start_date = cleaned_data.get("start_date") or default_dates.start
        end_date = cleaned_data.get("end_date") or default_dates.end

        if start_date > end_date:
            raise forms.ValidationError("Start date must not be after end date.")

        if (end_date - start_date).days > self.max_range_days:
            raise forms.ValidationError(
                f"Date range must not exceed {self.max_range_days} days."
            )

        cleaned_data["start_date"] = start_date
        cleaned_data["end_date"] = end_date
        return cleaned_data


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
