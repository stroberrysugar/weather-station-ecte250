The esp32-wind-speed-fix.ino file contains the code we used to present at the innovation fair

The esp32-wifi-changing-WIP.ino file contains the code that attempts to support switching WiFi networks (but still a WIP)

Both files have different fixes of their own, so before launching the product, the fixes must be merged into one and tested again.

In particular, the wifi changing code doesn't contain the wind speed fix.

I think you should be able to compile the esp32 code by just installing the libraries mentioned in the code. Here are the specific versions we used:

1. Adafruit Unified Sensor: 1.1.14
2. Adafruit BMP3XX: 2.1.5
3. Adafruit HMC5883: 1.2.3
4. ArduinoJson: 7.2.0
5. DHT sensor library: 1.4.6
6. DHTLib: 0.1.36

(I'm not sure which DHT library we used, so I just listed both of the ones I have installed. The code should still compile even if you have both installed)

Also make sure in board manager, you install esp32 version 3.0.0-rc1. We found out that in later versions, there's a problem with connecting to eduroam and other WPA2 enterprise networks (the board kept crashing). And so, we downgraded to that version and it started working again.

Just to be safe, I've uploaded my entire Arduino folder (Linux). Note to Linux users, try running it under the root user since it fixed problems with reading from /tty/USB0.

The Rust backend's source code (and binary) are in the ZIP file. However, it's still running on my main server and probably will be until my main server goes down (which is unlikely since I use it for work and other personal things). Even if I do take down my main server because of cost (understandable since it's like $100 a month), I'll probably migrate everything to a different server.
