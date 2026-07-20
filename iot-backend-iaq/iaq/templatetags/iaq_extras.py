from django import template

register = template.Library()

IAQ_LEVELS = (
    (100, "Good", "good"),
    (200, "Moderate", "moderate"),
    (350, "Poor", "poor"),
)


@register.filter
def iaq_status(value):
    try:
        value = float(value)
    except (TypeError, ValueError):
        return {"label": "No data", "css_class": "unknown"}
    for threshold, label, css_class in IAQ_LEVELS:
        if value <= threshold:
            return {"label": label, "css_class": css_class}
    return {"label": "Hazardous", "css_class": "hazardous"}


# WiFi RSSI (dBm) thresholds, roughly matching a phone's 4-bar signal meter.
RSSI_LEVELS = (
    (-55, "Excellent"),
    (-65, "Good"),
    (-75, "Fair"),
    (-85, "Weak"),
)


@register.filter
def rssi_signal(value):
    try:
        value = float(value)
    except (TypeError, ValueError):
        return "No data"
    for threshold, label in RSSI_LEVELS:
        if value >= threshold:
            return label
    return "Very weak"


@register.filter
def format_uptime(value):
    try:
        seconds = int(value)
    except (TypeError, ValueError):
        return "N/A"
    if seconds < 0:
        return "N/A"

    days, remainder = divmod(seconds, 86400)
    hours, remainder = divmod(remainder, 3600)
    minutes, seconds = divmod(remainder, 60)

    parts = []
    if days:
        parts.append(f"{days}d")
    if days or hours:
        parts.append(f"{hours}h")
    if days or hours or minutes:
        parts.append(f"{minutes}m")
    parts.append(f"{seconds}s")

    return " ".join(parts)


# Matches firmware's SensorMode enum (include/01_sensor/sensor_types.hpp).
SENSOR_MODE_LABELS = {
    0: "Disabled",
    1: "Ultra Low Power",
    2: "Low Power",
    3: "Continuous",
}


@register.filter
def sensor_mode_label(value):
    try:
        value = int(value)
    except (TypeError, ValueError):
        return "N/A"
    return SENSOR_MODE_LABELS.get(value, "N/A")
