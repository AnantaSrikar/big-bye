# Setup

## ESP32
To run this on an ESP32, make sure the following setting are configured on the Arduino IDE:

1. Install the libraries mentioned in [dependencies](/firmware/dependencies.txt)
2. Configure Arduino IDE as below:
![Arduino Config setting should be here, idk why it isn't :(](/res/ArduinoConfig.png)
3. Make sure to replace `SERVER_ADDR` with the address of your server.

I'm using an ESP32-WROOM-32 with 4MB Flash size, hence using the respective Partition Scehme to fit the application. Feel free to modify that as per your storage options.

## Web server
Head over [here](/web_server/) for setting up your webserver.