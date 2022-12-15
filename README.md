# SknLD2410
ESP32 Program that explores the LD2410 mmWave Human Detection Sensor

Example sketch for reporting on readings from the LD2410 using 
whatever settings are currently configured. The ability to change
the device configuration is enabled via command over Serial or UDP transports.

The sketch assumes an ESP32 board with the LD2410 connected as Serial2 
on pins 16 & 17, the serial configuration for other boards may vary. ESP8266 boards 
may also work but not tried.

The Program broadcasts [reporting data] on UDP port 8090, and listens on 
UDP port 8091 for commands or configuration requests, 
responses are sent to the callers ip/port as discovered.

Where [reporting data] is CSV-like with attributes as
documented in [SerialStudio's page](https://github.com/Serial-Studio/Serial-Studio/wiki/Communication-Protocol)

WIFI_SSID and WIFI_PASS are double-quoted environment variables with related values, A strategy used 
to avoid documenting WiFi credentials in the open.

-- examples: 
    export PLATFORMIO_BUILD_FLAGS=-DWIFI_PASS='"ssid-password"' -DWIFI_SSID='"ssid-value"'
or 
    export WIFI_PASS='"ssid-password"'
    export WIFI_SSID='"ssid-value"'


Gates: 
- each gate is 0.75m or 30 inches
 0 to 9 gates = 6.75m or 22 feet ish

Replace the IPAddress() values with IP Address of the machine running SerialStudio and deploy to ESP32.

![](ld2410andbreakout.jpg)

![](ld2410pinout.jpg)

The test directory of this repo contains a JSON file formatted for the [SerialStudio application](https://github.com/Serial-Studio/Serial-Studio). 
The provided ESP program and SerialStudio app provide deep understanding of the LD2410 operation, and 
provides the ability to configure the device to meet your objectives.

![](SerialStudio-Screenshot.png)


