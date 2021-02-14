#include "config.h"
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "hoermann.h"

WiFiClient espClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, espClient);
PubSubClientTools mqtt(client);

Hoermann door;
hoermann_state_t current_door_state = hoermann_state_unkown;
hoermann_state_t last_door_state = hoermann_state_unkown;

Adafruit_BME280 bme; // I2C
bool bme_detected = false;
uint32_t ReconnectCounter = 0;
uint32_t StartTime;

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(OTA_HOSTNAME);

  // No authentication by default
  ArduinoOTA.setPassword(OTA_PASSWORT);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.print("OTA ready - IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT
  Serial.print("Connecting to MQTT: ");
  Serial.print(MQTT_SERVER);
  Serial.print("...");
  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASSWORD, STATE_TOPIC, 0, true, "offline")) {
    Serial.println("connected");
    mqtt.subscribe(SET_TOPIC, topic1_subscriber);
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }

  bme_detected = bme.begin(BME280_I2C_ADR);
  if (bme_detected)
  {
    mqtt.publish(BME280_STATE_TOPIC, "online", false);
    Serial.println("BME connected!");
  }
  else
  {
    mqtt.publish(BME280_STATE_TOPIC, "offline", true);
    Serial.println("BME not found!");
  }

  Serial.flush();
  Serial.end();
  Serial.begin(19200);
  Serial.swap();

  StartTime = millis();
}

void loop()
{
  door.loop();

  if (WiFi.status() != WL_CONNECTED)
  {
    reconnect_wifi();
    ReconnectCounter++;
    last_door_state = hoermann_state_unkown;
  }
  else
  {
    if (!client.connected()) {
      reconnect_mqtt();
      ReconnectCounter++;
      last_door_state = hoermann_state_unkown;
    }
    else
    {
      client.loop();

      current_door_state = door.get_state();
      if (current_door_state != last_door_state)
      {
        String state = door.get_state_string();
        mqtt.publish(STATE_TOPIC, state, true);
        last_door_state = current_door_state;
      }

      if (bme_detected)
      {
        read_bme();
      }
      else
      {
        connect_bme();
      }

      if (ReconnectCounter > 0) {
        ReconnectCounter--;
      }
    }

    ArduinoOTA.handle();
  }

  if (ReconnectCounter > 100) {
    ESP.restart();
  }
}

void reconnect_wifi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  delay(500);
}

void reconnect_mqtt() {

  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASSWORD, STATE_TOPIC, 0, true, "offline")) {
    mqtt.subscribe(SET_TOPIC, topic1_subscriber);
  } else {
    delay(500);
  }
}

void read_bme(void)
{
  String message;
  uint32_t CurrentTime;

  CurrentTime = millis();
  if ((CurrentTime - StartTime) >= 30000)
  {
    StartTime = CurrentTime;

    Wire.beginTransmission(BME280_I2C_ADR);
    if (Wire.endTransmission() == 0)
    {
      message = "{ \"temperature_C\" : " + String(bme.readTemperature()) + ", \"humidity\" : " + String(bme.readHumidity()) + ", \"pressure_hPa\" : " + String(bme.readPressure() / 100.0F) + " }";
      mqtt.publish(BME280_VALUE_TOPIC, message, true);
    }
    else
    {
      mqtt.publish(BME280_STATE_TOPIC, "offline", true);
      bme_detected = false;
    }
  }
}

void connect_bme(void)
{
  String message;
  uint32_t CurrentTime;

  CurrentTime = millis();
  if ((CurrentTime - StartTime) >= 30000)
  {
    StartTime = CurrentTime;

    bme_detected = bme.begin(BME280_I2C_ADR);
    if (bme_detected)
    {
      mqtt.publish(BME280_STATE_TOPIC, "online", false);
    }
  }
}

void topic1_subscriber(String topic, String message)
{
  if (message == "OPEN")
  {
    door.trigger_action(hoermann_action_open);
  }
  if (message == "CLOSE")
  {
    door.trigger_action(hoermann_action_close);
  }
  if (message == "STOP")
  {
    door.trigger_action(hoermann_action_stop);
  }
  if (message == "VENTING")
  {
    door.trigger_action(hoermann_action_venting);
  }
}
