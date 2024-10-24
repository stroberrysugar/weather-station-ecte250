const socket = new WebSocket("/ws");

function setInnerText(elem, text) {
    document.getElementById(elem).innerText = text;
}

// value-temperature
// value-wind_speed
// value-wind_direction
// value-uv_index
// value-uv_index_rating
// value-humidity
// value-pressure

function getDirection(headingDegrees) {
    if (headingDegrees >= 337.5 || headingDegrees < 22.5) {
        return "N";
    } else if (headingDegrees >= 22.5 && headingDegrees < 67.5) {
        return "NE";
    } else if (headingDegrees >= 67.5 && headingDegrees < 112.5) {
        return "E";
    } else if (headingDegrees >= 112.5 && headingDegrees < 157.5) {
        return "SE";
    } else if (headingDegrees >= 157.5 && headingDegrees < 202.5) {
        return "S";
    } else if (headingDegrees >= 202.5 && headingDegrees < 247.5) {
        return "SW";
    } else if (headingDegrees >= 247.5 && headingDegrees < 292.5) {
        return "W";
    } else if (headingDegrees >= 292.5 && headingDegrees < 337.5) {
        return "NW";
    } else {
        return "Unknown"; // Fallback case, though should never happen with the current logic
    }
}

function getUvRating(uvIndex) {
    if (uvIndex >= 0 && uvIndex <= 2.9) {
        return "Low";
    } else if (uvIndex >= 3.0 && uvIndex <= 5.9) {
        return "Moderate";
    } else if (uvIndex >= 6.0 && uvIndex <= 7.9) {
        return "High";
    } else if (uvIndex >= 8.0 && uvIndex <= 10.9) {
        return "Very High";
    } else {
        return "Extreme";
    }
}

socket.addEventListener("open", (event) => {
    socket.send(JSON.stringify({
        type: "identify",
        data: {
            unique_device_id: "0a1c3bbf72",
            password: "test123"
        }
    }));
});

socket.addEventListener("message", (event) => {
    let json = JSON.parse(event.data);

    if (json.type == "keep_alive") {
        return;
    }

    if (json.type != "weather_data_event") {
        return;
    }

    let weather_values = json.data;

    /*
        pub struct WeatherValues {
            pub humidity: f64,
            pub temperature: f64,
            pub uv_index: f64,
            pub pressure: f64,
            pub wind_speed: f64,
            pub wind_direction: f64,
        }
    */

    setInnerText("value-humidity", weather_values.humidity);
    setInnerText("value-temperature", weather_values.temperature);
    setInnerText("value-uv_index", weather_values.uv_index);
    setInnerText("value-uv_index_rating", getUvRating(weather_values.uv_index));
    setInnerText("value-pressure", weather_values.pressure);
    setInnerText("value-wind_speed", weather_values.wind_speed);
    setInnerText("value-wind_direction", getDirection(weather_values.wind_direction));
});

socket.addEventListener("close", () => {
    window.location.reload();
});

socket.addEventListener("error", () => {
    window.location.reload();
});
