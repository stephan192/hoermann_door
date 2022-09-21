# Configuration
The configuration is stored in `esp8266\config.h`. If file doesn't exist copy `esp8266\config_template.h` and adopt to your needs.

| Define          | Description |
|-----------------|-------------|
| `HOSTNAME`      | Hostname used for WIFI, OTA and MQTT |
| `WIFI_SSID`     | WIFI ssid to which the ESP should connect |
| `WIFI_PASS`     | WIFI password used to connect to `WIFI_SSID` |
| `MQTT_SERVER`   | IP of the MQTT server to which the ESP should connect |
| `MQTT_PORT`     | Port of the MQTT server to which the ESP should connect (typically 1883) |
| `MQTT_USER`     | Username used to connect to the MQTT server |
| `MQTT_PASSWORD` | Password used to connect to the MQTT server |
| `OTA_PASSWORT`  | Password used for over the air updates from Arduino IDE |
| `BME280_I2C_ADR`| I2C address of the BME280 |
