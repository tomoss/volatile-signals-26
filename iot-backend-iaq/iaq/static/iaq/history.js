(function () {
    const startInput = document.getElementById("start_date");
    const endInput = document.getElementById("end_date");

    function syncBounds() {
        endInput.min = startInput.value;
        startInput.max = endInput.value;
    }
    syncBounds();
    startInput.addEventListener("change", syncBounds);
    endInput.addEventListener("change", syncBounds);

    const sensorReadingsData = document.getElementById("sensor_readings_data");
    if (!sensorReadingsData) {
        return;
    }

    const sensorReadings = JSON.parse(sensorReadingsData.textContent);
    const labels = sensorReadings.map(function (r) {
        return new Date(r.timestamp).toLocaleString([], {
            month: "short",
            day: "numeric",
            hour: "2-digit",
            minute: "2-digit",
        });
    });
    const showMarkers = sensorReadings.length <= 60;

    const readingTimes = sensorReadings.map(function (r) {
        return new Date(r.timestamp).getTime();
    });

    const GAP_WINDOW_SIZE = 5;

    const deltas = [];
    for (let i = 1; i < readingTimes.length; i++) {
        deltas.push(readingTimes[i] - readingTimes[i - 1]);
    }

    function median(values) {
        const sorted = values.slice().sort(function (a, b) { return a - b; });
        return sorted[Math.floor(sorted.length / 2)];
    }

    function localGapThreshold(deltaIndex) {
        const window = [];
        for (let i = Math.max(0, deltaIndex - GAP_WINDOW_SIZE); i < deltaIndex; i++) {
            window.push(deltas[i]);
        }
        for (
            let i = deltaIndex + 1;
            i <= Math.min(deltas.length - 1, deltaIndex + GAP_WINDOW_SIZE);
            i++
        ) {
            window.push(deltas[i]);
        }
        if (window.length === 0) {
            return Infinity;
        }
        return median(window) * 2;
    }

    function isGapSegment(ctx) {
        const deltaIndex = ctx.p0DataIndex;
        return deltas[deltaIndex] > localGapThreshold(deltaIndex);
    }

    // Bosch BSEC IAQ scale (Excellent..Extremely polluted, 0-500+), collapsed
    // into the app's 4-step status palette without splitting any of its bands:
    // good = Excellent+Good, warning = Lightly+Moderately polluted,
    // serious = Heavily+Severely polluted, critical = Extremely polluted.
    const IAQ_STATUS_COLORS = {
        good: "#22c55e",
        warning: "#f5a623",
        serious: "#f97316",
        critical: "#ef4477",
    };

    function iaqStatusColor(value) {
        if (value <= 100) {
            return IAQ_STATUS_COLORS.good;
        }
        if (value <= 200) {
            return IAQ_STATUS_COLORS.warning;
        }
        if (value <= 350) {
            return IAQ_STATUS_COLORS.serious;
        }
        return IAQ_STATUS_COLORS.critical;
    }

    function iaqSegmentColor(ctx) {
        if (isGapSegment(ctx)) {
            return "transparent";
        }
        return iaqStatusColor(sensorReadings[ctx.p1DataIndex].iaq);
    }

    function hexToRgba(hex, alpha) {
        const r = parseInt(hex.slice(1, 3), 16);
        const g = parseInt(hex.slice(3, 5), 16);
        const b = parseInt(hex.slice(5, 7), 16);
        return "rgba(" + r + ", " + g + ", " + b + ", " + alpha + ")";
    }

    function iaqSegmentFillColor(ctx) {
        if (isGapSegment(ctx)) {
            return "transparent";
        }
        return hexToRgba(iaqStatusColor(sensorReadings[ctx.p1DataIndex].iaq), 0.2);
    }

    const gridColor = "#232c42";
    const tickColor = "#8b93a7";

    const metrics = [
        {key: "iaq", canvasId: "chart-iaq", label: "IAQ", color: "#2a78d6"},
        {key: "co2_equivalent", canvasId: "chart-co2", label: "CO2 equivalent (ppm)", color: "#eb6834"},
        {key: "voc_equivalent", canvasId: "chart-voc", label: "VOC equivalent (ppb)", color: "#1baf7a"},
        {key: "temperature", canvasId: "chart-temperature", label: "Temperature (°C)", color: "#eda100"},
        {key: "humidity", canvasId: "chart-humidity", label: "Humidity (%)", color: "#e87ba4"},
        {key: "pressure", canvasId: "chart-pressure", label: "Pressure (hPa)", color: "#008300"},
    ];

    function createChart(metric) {
        const canvas = document.getElementById(metric.canvasId);
        if (!canvas) {
            return;
        }
        new Chart(canvas, {
            type: "line",
            data: {
                labels: labels,
                datasets: [{
                    label: metric.label,
                    data: sensorReadings.map(function (r) { return r[metric.key]; }),
                    borderColor: metric.color,
                    backgroundColor: metric.color,
                    pointBackgroundColor: metric.key === "iaq"
                        ? sensorReadings.map(function (r) { return iaqStatusColor(r.iaq); })
                        : metric.color,
                    borderWidth: 2,
                    pointRadius: showMarkers ? 4 : 0,
                    pointHoverRadius: 5,
                    tension: 0.2,
                    fill: metric.key === "iaq" ? "start" : false,
                    segment: {
                        borderColor: metric.key === "iaq"
                            ? iaqSegmentColor
                            : function (ctx) {
                                return isGapSegment(ctx) ? "transparent" : undefined;
                            },
                        backgroundColor: metric.key === "iaq" ? iaqSegmentFillColor : undefined,
                    },
                }],
            },
            options: {
                responsive: true,
                aspectRatio: metric.key === "iaq" ? 4 : 2,
                interaction: {mode: "index", intersect: false},
                plugins: {
                    legend: {display: false},
                    tooltip: {mode: "index", intersect: false},
                },
                scales: {
                    x: {
                        grid: {color: gridColor},
                        ticks: {color: tickColor, maxRotation: 0, autoSkip: true, autoSkipPadding: 30},
                    },
                    y: {
                        grid: {color: gridColor},
                        ticks: {color: tickColor},
                    },
                },
            },
        });
    }

    metrics.forEach(createChart);
})();
