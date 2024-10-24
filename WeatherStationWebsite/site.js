let TEMPERATURE = 15;

function changeMap(map) {
    const map1 = document.getElementById("map1");
    const map2 = document.getElementById("map2");

    switch (map) {
        case "device":
            map1.classList.add("visible");
            map2.classList.remove("visible");
            break;
        case "wind":
            map2.classList.add("visible");
            map1.classList.remove("visible");
            break;
    }

    window.dispatchEvent(new Event('resize'));
}