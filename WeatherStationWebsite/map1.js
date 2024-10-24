mapboxgl.accessToken = 'pk.eyJ1IjoiZnJvZzEyMzQiLCJhIjoiY20xZzRuOWI2MDA0azJ2cHlqb3Q2c3ljZyJ9.v9PVnTO59ck3XchIMIYVAA';

const geojson = {
    'type': 'FeatureCollection',
    'features': [
        {
            'type': 'Feature',
            'properties': {
                'message': 'Foo',
                'imageId': '7eaab4cabf57f472e83a6da5a4828586',
                'iconSize': [60, 60]
            },
            'geometry': {
                'type': 'Point',
                'coordinates': [150.874522, -34.410568]
            }
        },
        {
            'type': 'Feature',
            'properties': {
                'message': 'Bar',
                'imageId': 'd2248ea2c4fe61900cd655ac427f1d40',
                'iconSize': [50, 50]
            },
            'geometry': {
                'type': 'Point',
                'coordinates': [150.872708, -34.406014]
            }
        },
        {
            'type': 'Feature',
            'properties': {
                'message': 'Baz',
                'imageId': 'de55016cb785f1137062a01019167162',
                'iconSize': [40, 40]
            },
            'geometry': {
                'type': 'Point',
                'coordinates': [150.893146, -34.426072]
            }
        }
    ]
};

const map = new mapboxgl.Map({
    container: 'map1',
    style: 'mapbox://styles/frog1234/cm1g4k04m004q01pr1e4s1o0q',
    center: [150.879591, -34.407790],
    zoom: 13
});

const size = 150;

const pulsingDot = {
    width: size,
    height: size,
    data: new Uint8Array(size * size * 4),

    onAdd: function () {
        const canvas = document.createElement('canvas');
        canvas.width = this.width;
        canvas.height = this.height;
        this.context = canvas.getContext('2d');
    },

    render: function () {
        const totalDuration = 3000; // Total time between pulses (3 seconds)
        const pulseDuration = 1000; // Duration of the pulse animation (1 second)
        const timeNow = performance.now();
        const elapsed = timeNow % totalDuration;

        let t;

        if (elapsed <= pulseDuration) {
            // Animate the pulse
            t = elapsed / pulseDuration;
        } else {
            // Keep the dot static
            t = 0;
        }

        const radius = (size / 2) * 0.3;
        const outerRadius = (size / 2) * 0.7 * t + radius;
        const context = this.context;

        // Clear the canvas
        context.clearRect(0, 0, this.width, this.height);

        // Draw the outer circle only during the pulse animation
        if (t > 0) {
            context.beginPath();
            context.arc(
                this.width / 2,
                this.height / 2,
                outerRadius,
                0,
                Math.PI * 2
            );
            context.fillStyle = `rgba(255, 200, 200, ${1 - t})`;
            context.fill();
        }

        // Draw the inner circle (static dot)
        context.beginPath();
        context.arc(
            this.width / 2,
            this.height / 2,
            radius,
            0,
            Math.PI * 2
        );
        context.fillStyle = 'rgba(255, 100, 100, 1)';
        context.strokeStyle = 'white';
        context.lineWidth = 2;
        context.fill();
        context.stroke();

        // Update the image data
        this.data = context.getImageData(
            0,
            0,
            this.width,
            this.height
        ).data;

        // Trigger map repaint
        map.triggerRepaint();

        return true;
    }
};

map.on('load', () => {
    map.addImage('pulsing-dot', pulsingDot, { pixelRatio: 2 });

    map.addSource('dot-point', {
        'type': 'geojson',
        'data': {
            'type': 'FeatureCollection',
            'features': [
                {
                    'type': 'Feature',
                    'geometry': {
                        'type': 'Point',
                        'coordinates': [150.879591, -34.407790]
                    }
                }
            ]
        }
    });
    map.addLayer({
        'id': 'layer-with-pulsing-dot',
        'type': 'symbol',
        'source': 'dot-point',
        'layout': {
            'icon-image': 'pulsing-dot'
        }
    });
});

for (const marker of geojson.features) {
    // Create a DOM element for each marker.
    const el = document.createElement('div');
    const width = marker.properties.iconSize[0];
    const height = marker.properties.iconSize[1];
    el.className = 'map-device-marker';
    el.style.backgroundImage = `url(/img/${marker.properties.imageId}.png)`;
    el.style.width = `${width}px`;
    el.style.height = `${height}px`;
    el.style.backgroundSize = '100%';

    // el.addEventListener('click', () => {
    //     window.alert(marker.properties.message);
    // });

    // Add markers to the map.
    new mapboxgl.Marker(el)
        .setLngLat(marker.geometry.coordinates)
        .addTo(map);
}