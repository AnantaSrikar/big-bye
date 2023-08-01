# ESP32 Firmware

This firmware runs on the ESP32, which manages the Wi-Fi connection and fetches messages from a web server. These messages are then displayed on a TFT display.

## Features:
1. Dynamic WiFi Provisioning using ESP SoftAP Provisioning, available on both [Android](https://play.google.com/store/apps/details?id=com.espressif.provsoftap&hl=en&gl=US) and [iOS](https://apps.apple.com/ms/app/esp-softap-provisioning/id1474040630).
2. Dynamically loads the heading and messages onto the screen.
3. Support for combo button presses. Currently, UP + DOWN buttons work for resetting the board.
4. Works with BasicAuth for increased user privacy.

## Arduino IDE Setup
To run this on an ESP32, make sure the following settings are configured in the Arduino IDE:

1. Install the libraries mentioned in [dependencies.txt](/firmware/dependencies.txt).
2. Configure the Arduino IDE as shown below:

![Arduino Config setting should be here, but not sure why it isn't :(](/res/ArduinoConfig.png)

I'm using an ESP32-WROOM-32 with a 4MB Flash size; hence, I'm using the respective Partition Scheme to fit the application. Feel free to modify that as per your storage options.

## Flashing Firmware:
1. Set up your connection to the ESP32 via its UART port to flash it.
2. Make sure the ESP32 is booted in the Firmware Download Mode.
3. Select the right serial port in the Arduino IDE.
4. Modify `USERNAME`, `SERVER_ADDR`, `BASICAUTH_USERNAME`, and `BASICAUTH_PASSWORD` according to your [web server](../web_server) configuration.
5. Compile and wait for the flashing to be done.

## User Privacy
To ensure user privacy, the web server will be protected with BasicAuth using [Caddy](https://caddyserver.com/).

However, assigning usernames that are hard to guess will further increase privacy. For example, "somerandomdude-42069" would be hard to guess.

## Web Server
Head over [here](/web_server/) to set up your web server.