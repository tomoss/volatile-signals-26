(function () {
    var HISTORY_LENGTH = 5;
    var SVG_NS = "http://www.w3.org/2000/svg";
    var buffers = {};

    function pushValue(key, rawValue) {
        if (rawValue === "" || rawValue === null) {
            return;
        }
        var value = parseFloat(rawValue);
        if (isNaN(value)) {
            return;
        }
        var buffer = buffers[key] || (buffers[key] = []);
        buffer.push(value);
        if (buffer.length > HISTORY_LENGTH) {
            buffer.shift();
        }
    }

    function sparklinePoints(values, width, height) {
        if (!values.length) {
            return null;
        }
        var vals = values.length > 1 ? values : values.concat(values);
        var vmin = Math.min.apply(null, vals);
        var vmax = Math.max.apply(null, vals);
        var range = (vmax - vmin) || 1;
        var step = width / (vals.length - 1);
        var coords = vals.map(function (v, i) {
            var x = i * step;
            var y = height - ((v - vmin) / range) * height;
            return x.toFixed(1) + "," + y.toFixed(1);
        });
        var line = coords.join(" ");
        var area = "0," + height.toFixed(1) + " " + line + " " + width.toFixed(1) + "," + height.toFixed(1);
        return {line: line, area: area};
    }

    function renderSparkline(svg, points, color) {
        while (svg.firstChild) {
            svg.removeChild(svg.firstChild);
        }
        if (!points) {
            return;
        }
        var polygon = document.createElementNS(SVG_NS, "polygon");
        polygon.setAttribute("points", points.area);
        polygon.setAttribute("fill", color);
        polygon.setAttribute("opacity", "0.12");
        var polyline = document.createElementNS(SVG_NS, "polyline");
        polyline.setAttribute("points", points.line);
        polyline.setAttribute("fill", "none");
        polyline.setAttribute("stroke", color);
        polyline.setAttribute("stroke-width", "1.5");
        polyline.setAttribute("stroke-linecap", "round");
        polyline.setAttribute("stroke-linejoin", "round");
        svg.appendChild(polygon);
        svg.appendChild(polyline);
    }

    function updateSparklines(root) {
        (root || document).querySelectorAll("[data-sparkline]").forEach(function (svg) {
            var key = svg.getAttribute("data-sparkline");
            pushValue(key, svg.getAttribute("data-value"));
            var points = sparklinePoints(buffers[key] || [], 100, 32);
            renderSparkline(svg, points, svg.getAttribute("data-color") || "#22d3ee");
        });
    }

    document.addEventListener("DOMContentLoaded", function () {
        updateSparklines();
    });
    document.body.addEventListener("htmx:afterSettle", function (evt) {
        updateSparklines(evt.target);
    });
})();
