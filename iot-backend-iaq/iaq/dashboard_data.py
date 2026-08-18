import math

from iaq.templatetags.iaq_extras import iaq_status

# Keep in sync with the status-* color variables in iaq/static/iaq/css/base.css.
STATUS_COLORS = {
    "good": "#22c55e",
    "moderate": "#f5a623",
    "poor": "#f97316",
    "hazardous": "#ef4477",
    "unknown": "#8b93a7",
}

# Bosch BSEC IAQ scale (7 tiers), for the IAQ gauge only. Keep in sync with
# iaq_extras.iaq_status and the status-* color variables in base.css.
IAQ_STATUS_COLORS = {
    "excellent": "#10b981",
    "good": "#22c55e",
    "lightly-polluted": "#eab308",
    "moderately-polluted": "#f97316",
    "heavily-polluted": "#ef4444",
    "severely-polluted": "#be123c",
    "extremely-polluted": "#78350f",
    "unknown": "#8b93a7",
}

METRIC_DEFS = [
    {
        "key": "temperature",
        "label": "Temperature",
        "unit": "°C",
        "icon": "bi-thermometer-half",
        "description": "Ideal range 18–24 °C for comfort",
        "band": (18, 24),
        "range": (10, 40),
        "decimals": 1,
    },
    {
        "key": "humidity",
        "label": "Humidity",
        "unit": "%",
        "icon": "bi-droplet",
        "description": "Optimal 40–60% relative humidity",
        "band": (40, 60),
        "range": (0, 100),
        "decimals": 0,
    },
    {
        "key": "pressure",
        "label": "Pressure",
        "unit": "hPa",
        "icon": "bi-speedometer2",
        "description": "Standard sea level ~1013 hPa",
        "status": "good",
        "range": (980, 1050),
        "decimals": 0,
    },
    {
        "key": "co2_equivalent",
        "label": "CO₂ equivalent",
        "unit": "ppm",
        "icon": "bi-cloud-haze2",
        "description": "Outdoor baseline ~420 ppm; >1000 impairs cognition",
        "status": "unknown",
        "range": (400, 2000),
        "decimals": 0,
    },
    {
        "key": "voc_equivalent",
        "label": "VOC equivalent",
        "unit": "ppb",
        "icon": "bi-activity",
        "description": "Volatile organic compounds from surfaces & products",
        "status": "unknown",
        "range": (0, 500),
        "decimals": 0,
    },
]

GAUGE_MAX = 500
GAUGE_CX, GAUGE_CY, GAUGE_R, GAUGE_TICK_INNER = 90, 90, 70, 56
GAUGE_ARC = math.pi * GAUGE_R
GAUGE_TICKS = [
    {
        "x1": GAUGE_CX - GAUGE_R * math.cos(math.pi * f),
        "y1": GAUGE_CY - GAUGE_R * math.sin(math.pi * f),
        "x2": GAUGE_CX - GAUGE_TICK_INNER * math.cos(math.pi * f),
        "y2": GAUGE_CY - GAUGE_TICK_INNER * math.sin(math.pi * f),
    }
    for f in (0, 0.17, 0.33, 0.5, 0.67, 0.83, 1)
]


def _band_status(value, band):
    low, high = band
    return "good" if low <= value <= high else "poor"


def build_dashboard_context(device):
    latest = device.latest_sensor_reading

    metrics = []
    for metric_def in METRIC_DEFS:
        key = metric_def["key"]
        value = getattr(latest, key) if latest else None
        if value is None:
            status = "unknown"
        elif "status" in metric_def:
            status = metric_def["status"]
        else:
            status = _band_status(value, metric_def["band"])
        color = STATUS_COLORS[status]
        lo, hi = metric_def["range"]
        percent = (
            round(max(2, min(100, ((value - lo) / (hi - lo)) * 100)), 1)
            if value is not None
            else 0
        )
        metrics.append(
            {
                "key": key,
                "label": metric_def["label"],
                "unit": metric_def["unit"],
                "icon": metric_def["icon"],
                "description": metric_def["description"],
                "value": round(value, metric_def["decimals"]) if value is not None else None,
                "status": status,
                "color": color,
                "percent": percent,
            }
        )

    score = round(latest.iaq) if latest else None
    gauge_status = iaq_status(score)
    pct = min(score / GAUGE_MAX, 1) if score is not None else 0
    gauge = {
        "score": score,
        "status_label": gauge_status["label"],
        "color": IAQ_STATUS_COLORS[gauge_status["css_class"]],
        "arc": round(GAUGE_ARC, 2),
        "dash_offset": round(GAUGE_ARC * (1 - pct), 2),
    }

    return {
        "metrics": metrics,
        "gauge": gauge,
        "gauge_ticks": GAUGE_TICKS,
    }
