from django import template

register = template.Library()

# Bosch BSEC IAQ scale.
IAQ_LEVELS = (
    (50, "Excellent", "excellent"),
    (100, "Good", "good"),
    (150, "Lightly polluted", "lightly-polluted"),
    (200, "Moderately polluted", "moderately-polluted"),
    (250, "Heavily polluted", "heavily-polluted"),
    (350, "Severely polluted", "severely-polluted"),
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
    return {"label": "Extremely polluted", "css_class": "extremely-polluted"}


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


# BSEC IAQ accuracy indicator: 0 = still stabilizing, 3 = fully calibrated.
ACCURACY_LABELS = {
    0: "Uncalibrated",
    1: "Low",
    2: "Medium",
    3: "High",
}


@register.filter
def accuracy_label(value):
    try:
        value = int(value)
    except (TypeError, ValueError):
        return "N/A"
    return ACCURACY_LABELS.get(value, "N/A")
