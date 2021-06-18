# Configuration
The configuration is stored in `esp8266\config.h`. If file doesn't exist copy `esp8266\config_template.h` and adopt to your needs.

# Basics
| Define          | Description |
|-----------------|-------------|
| `WIFI_SSID`     | WIFI ssid to which the ESP should connect |
| `WIFI_PASS`     | WIFI password used to connect to `WIFI_SSID` |
| `MQTT_SERVER`   | IP of the MQTT server to which the ESP should connect |
| `MQTT_PORT`     | Port of the MQTT server to which the ESP should connect (typically 1883) |
| `MQTT_CLIENTID` | Unique id to identify your MQTT client (e.g. doordrive) |
| `MQTT_USER`     | Username used to connect to the MQTT server |
| `MQTT_PASSWORD` | Password used to connect to the MQTT server |
| `OTA_HOSTNAME`  | Hostname used for over the air updates from Arduino IDE |
| `OTA_PASSWORT`  | Password used for over the air updates from Arduino IDE |
| `BME280_I2C_ADR`| I2C address of the BME280 |

# MQTT
| Define               | Description |
|----------------------|-------------|
| `STATE_TOPIC`        | MQTT topic to which the current state of the door is published (content type: `string`, possible values: `open`, `closed`, `venting`, `opening`, `closing`, `error`, `stopped`) |
| `SET_TOPIC`          | Subscribed MQTT topic to retrieve commands (content type: `string`, possible values: `OPEN`, `CLOSE`, `STOP`, `VENTING`, `TOGGLE_LIGHT`) |
| `BME280_VALUE_TOPIC` | MQTT topic to which the current value of the BME280 is published (content type: `JSON string`, example: `{ "temperature_C" : 8.24, "humidity" : 66.26, "pressure_hPa" : 986.41 }`) |
| `BME280_STATE_TOPIC` | MQTT topic to which the current state of the BME280 is published (content type: `string`, possible values: `online`, `offline`) |
