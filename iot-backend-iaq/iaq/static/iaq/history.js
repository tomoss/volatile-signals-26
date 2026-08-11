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

    const gridColor = "#e1e0d9";
    const tickColor = "#898781";

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
                    borderWidth: 2,
                    pointRadius: showMarkers ? 4 : 0,
                    pointHoverRadius: 5,
                    tension: 0.2,
                    segment: {
                        borderColor: function (ctx) {
                            return isGapSegment(ctx) ? "transparent" : undefined;
                        },
                    },
                }],
            },
            options: {
                responsive: true,
                interaction: {mode: "index", intersect: false},
                plugins: {
                    legend: {display: false},
                    tooltip: {mode: "index", intersect: false},
                },
                scales: {
                    x: {
                        grid: {color: gridColor},
                        ticks: {color: tickColor, maxRotation: 0, autoSkip: true},
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
