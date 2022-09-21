#include "config.h"
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "hoermann.h"

#define HW_VERSION "v1"
#define SW_VERSION "v2"

WiFiClient espClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, espClient);
PubSubClientTools mqtt(client);
String unique_id;

Hoermann door;
hoermann_state_t current_door_state;
hoermann_state_t last_door_state;

Adafruit_BME280 bme; // I2C
bool bme_detected = false;
uint32_t ReconnectCounter = 0;
uint32_t StartTime;

String cover_avty_topic;
String cover_cmd_topic;
String cover_pos_topic;
String venting_cmd_topic;
String venting_state_topic;
String light_cmd_topic;
String light_state_topic;
String error_state_topic;
String prewarn_state_topic;
String option_relay_state_topic;
String bme_avty_topic;
String bme_state_topic;

void setup() {
  last_door_state.data_valid = false;

  Serial.begin(115200);
  Serial.println();

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAME);

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
  byte mac[6];
  WiFi.macAddress(mac);
  unique_id = "hoermann_door_" + String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  unique_id.toLowerCase();
  Serial.print("MQTT client id: ");
  Serial.println(unique_id);
  setup_mqtt_topics();

  Serial.print("Connecting to MQTT: ");
  Serial.print(MQTT_SERVER);
  Serial.print("...");
  if (client.connect(unique_id.c_str(), MQTT_USER, MQTT_PASSWORD, cover_avty_topic.c_str(), 0, true, "offline")) {
    Serial.println("connected");
    publish_mqtt_autodiscovery();
    mqtt_init_publish_and_subscribe();
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }

  bme_detected = bme.begin(BME280_I2C_ADR);
  if (bme_detected)
  {
    mqtt.publish(bme_avty_topic, "online", false);
    Serial.println("BME connected!");
  }
  else
  {
    mqtt.publish(bme_avty_topic, "offline", true);
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
    last_door_state.data_valid = false;
  }
  else
  {
    if (!client.connected()) {
      reconnect_mqtt();
      ReconnectCounter++;
      last_door_state.data_valid = false;
    }
    else
    {
      client.loop();

      process_door_data();

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

  if (client.connect(unique_id.c_str(), MQTT_USER, MQTT_PASSWORD, cover_avty_topic.c_str(), 0, true, "offline")) {
    mqtt_init_publish_and_subscribe();
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
      mqtt.publish(bme_state_topic, message, true);
    }
    else
    {
      mqtt.publish(bme_avty_topic, "offline", true);
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
      mqtt.publish(bme_avty_topic, "online", false);
    }
  }
}

void process_door_data() {
  current_door_state = door.get_state();
  /* Terminate if data is not valid */
  if (!current_door_state.data_valid)
  {
    return;
  }

  if ((current_door_state.cover != last_door_state.cover) || (!last_door_state.data_valid))
  {
    String position_message;
    switch (current_door_state.cover)
    {
      case cover_open:
        position_message = "100";
        break;
      case cover_closed:
        position_message = "0";
        break;
      default:
        position_message = "10";
        break;
    }
    mqtt.publish(cover_pos_topic, position_message, true);
    last_door_state.cover = current_door_state.cover;
  }
  if ((current_door_state.venting != last_door_state.venting) || (!last_door_state.data_valid))
  {
    String state_message;
    if (current_door_state.venting)
    {
      state_message = "ON";
    }
    else
    {
      state_message = "OFF";
    }
    mqtt.publish(venting_state_topic, state_message, true);
    last_door_state.venting = current_door_state.venting;
  }
  if ((current_door_state.light != last_door_state.light) || (!last_door_state.data_valid))
  {
    String state_message;
    if (current_door_state.light)
    {
      state_message = "ON";
    }
    else
    {
      state_message = "OFF";
    }
    mqtt.publish(light_state_topic, state_message, true);
    last_door_state.light = current_door_state.light;
  }
  if ((current_door_state.error != last_door_state.error) || (!last_door_state.data_valid))
  {
    String state_message;
    if (current_door_state.error)
    {
      state_message = "ON";
    }
    else
    {
      state_message = "OFF";
    }
    mqtt.publish(error_state_topic, state_message, true);
    last_door_state.error = current_door_state.error;
  }
  if ((current_door_state.prewarn != last_door_state.prewarn) || (!last_door_state.data_valid))
  {
    String state_message;
    if (current_door_state.prewarn)
    {
      state_message = "ON";
    }
    else
    {
      state_message = "OFF";
    }
    mqtt.publish(prewarn_state_topic, state_message, true);
    last_door_state.prewarn = current_door_state.prewarn;
  }
  if ((current_door_state.option_relay != last_door_state.option_relay) || (!last_door_state.data_valid))
  {
    String state_message;
    if (current_door_state.option_relay)
    {
      state_message = "ON";
    }
    else
    {
      state_message = "OFF";
    }
    mqtt.publish(option_relay_state_topic, state_message, true);
    last_door_state.option_relay = current_door_state.option_relay;
  }
  last_door_state.data_valid = current_door_state.data_valid;
}

void setup_mqtt_topics() {
  cover_avty_topic = "homeassistant/cover/" + unique_id + "_cover/availability";
  cover_cmd_topic = "homeassistant/cover/" + unique_id + "_cover/command";
  cover_pos_topic = "homeassistant/cover/" + unique_id + "_cover/position";

  venting_cmd_topic = "homeassistant/switch/" + unique_id + "_venting/command";
  venting_state_topic = "homeassistant/switch/" + unique_id + "_venting/state";

  light_cmd_topic = "homeassistant/switch/" + unique_id + "_light/command";
  light_state_topic = "homeassistant/switch/" + unique_id + "_light/state";

  error_state_topic = "homeassistant/binary_sensor/" + unique_id + "_error/state";
  prewarn_state_topic = "homeassistant/binary_sensor/" + unique_id + "_prewarn/state";
  option_relay_state_topic = "homeassistant/binary_sensor/" + unique_id + "_option_relay/state";

  bme_avty_topic = "homeassistant/sensor/" + unique_id + "_bme/availability";
  bme_state_topic = "homeassistant/sensor/" + unique_id + "_bme/state";
}

void mqtt_init_publish_and_subscribe() {
  mqtt.subscribe(cover_cmd_topic, cover_cmd_subscriber);
  mqtt.subscribe(venting_cmd_topic, venting_cmd_subscriber);
  mqtt.subscribe(light_cmd_topic, light_cmd_subscriber);

  mqtt.publish(cover_avty_topic, "online", true);
}

void publish_mqtt_autodiscovery() {
  String topic;
  String payload;

  String device = "\"dev\":{\"ids\":\"" + unique_id + "\", \"name\":\"" + HOSTNAME + "\", \"mdl\":\"Hoermann Door\", \"mf\":\"stephan192\", \"hw\":\"" + String(HW_VERSION) + "\", \"sw\":\"" + String(SW_VERSION) + "\"}";
  String obj_id = String(HOSTNAME);
  obj_id.toLowerCase();

  topic = "homeassistant/cover/" + unique_id + "_cover/config";
  payload = "{\"~\":\"homeassistant/cover/" + unique_id + "_cover\", \"avty_t\":\"~/availability\", \"cmd_t\":\"~/command\", " + device + ", \"dev_cla\":\"garage\", \"name\":\"Garage door\", \"obj_id\":\"" + obj_id + "_cover\", \"pos_t\":\"~/position\", \"uniq_id\":\"" + unique_id + "_cover\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/switch/" + unique_id + "_venting/config";
  payload = "{\"~\":\"homeassistant/switch/" + unique_id + "_venting\", \"avty_t\":\"" + cover_avty_topic + "\", \"cmd_t\":\"~/command\", " + device + ", \"icon\":\"mdi:hvac\", \"name\":\"Venting\", \"obj_id\":\"" + obj_id + "_venting\", \"stat_t\":\"~/state\", \"uniq_id\":\"" + unique_id + "_venting\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/switch/" + unique_id + "_light/config";
  payload = "{\"~\":\"homeassistant/switch/" + unique_id + "_light\", \"avty_t\":\"" + cover_avty_topic + "\", \"cmd_t\":\"~/command\", " + device + ", \"icon\":\"mdi:lightbulb\", \"name\":\"Light\", \"obj_id\":\"" + obj_id + "_light\", \"stat_t\":\"~/state\", \"uniq_id\":\"" + unique_id + "_light\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/binary_sensor/" + unique_id + "_error/config";
  payload = "{\"~\":\"homeassistant/binary_sensor/" + unique_id + "_error\", \"avty_t\":\"" + cover_avty_topic + "\", " + device + ", \"dev_cla\":\"problem\", \"name\":\"Error\", \"obj_id\":\"" + obj_id + "_error\", \"stat_t\":\"~/state\", \"uniq_id\":\"" + unique_id + "_error\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/binary_sensor/" + unique_id + "_prewarn/config";
  payload = "{\"~\":\"homeassistant/binary_sensor/" + unique_id + "_prewarn\", \"avty_t\":\"" + cover_avty_topic + "\", " + device + ", \"dev_cla\":\"safety\", \"name\":\"Prewarn\", \"obj_id\":\"" + obj_id + "_prewarn\", \"stat_t\":\"~/state\", \"uniq_id\":\"" + unique_id + "_prewarn\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/binary_sensor/" + unique_id + "_option_relay/config";
  payload = "{\"~\":\"homeassistant/binary_sensor/" + unique_id + "_option_relay\", \"avty_t\":\"" + cover_avty_topic + "\", " + device + ", \"name\":\"Option relay\", \"obj_id\":\"" + obj_id + "_option_relay\", \"stat_t\":\"~/state\", \"uniq_id\":\"" + unique_id + "_option_relay\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/sensor/" + unique_id + "_temperature/config";
  payload = "{\"avty\":[{\"topic\":\"" + cover_avty_topic + "\"}, {\"topic\":\"" + bme_avty_topic + "\"}], \"avty_mode\":\"all\", " + device + ", \"dev_cla\":\"temperature\", \"name\":\"Temperature\", \"obj_id\":\"" + obj_id + "_temperature\", \"stat_cla\":\"measurement\", \"stat_t\":\"" + bme_state_topic + "\", \"uniq_id\":\"" + unique_id + "_temperature\", \"unit_of_meas\":\"\u00b0C\", \"val_tpl\":\"{{value_json.temperature_C|round(1)}}\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/sensor/" + unique_id + "_humidity/config";
  payload = "{\"avty\":[{\"topic\":\"" + cover_avty_topic + "\"}, {\"topic\":\"" + bme_avty_topic + "\"}], \"avty_mode\":\"all\", " + device + ", \"dev_cla\":\"humidity\", \"name\":\"Humidity\", \"obj_id\":\"" + obj_id + "_humidity\", \"stat_cla\":\"measurement\", \"stat_t\":\"" + bme_state_topic + "\", \"uniq_id\":\"" + unique_id + "_humidity\", \"unit_of_meas\":\"%\", \"val_tpl\":\"{{value_json.humidity|round(0)}}\"}";
  publish_oversize_payload(topic, payload, true);

  topic = "homeassistant/sensor/" + unique_id + "_pressure/config";
  payload = "{\"avty\":[{\"topic\":\"" + cover_avty_topic + "\"}, {\"topic\":\"" + bme_avty_topic + "\"}], \"avty_mode\":\"all\", " + device + ", \"dev_cla\":\"pressure\", \"name\":\"Pressure\", \"obj_id\":\"" + obj_id + "_pressure\", \"stat_cla\":\"measurement\", \"stat_t\":\"" + bme_state_topic + "\", \"uniq_id\":\"" + unique_id + "_pressure\", \"unit_of_meas\":\"hPa\", \"val_tpl\":\"{{value_json.pressure_hPa|round(1)}}\"}";
  publish_oversize_payload(topic, payload, true);

  Serial.println("MQTT autodiscovery sent");
}

void publish_oversize_payload(String topic, String payload, bool retain)
{
  String substr;
  unsigned int index = 0u;
  unsigned int payload_len = payload.length();
  unsigned int count;

  client.beginPublish(topic.c_str(), payload.length(), retain);
  while (index < payload_len)
  {
    count = payload_len - index;
    if (count > MQTT_MAX_PACKET_SIZE) count = MQTT_MAX_PACKET_SIZE;

    substr = payload.substring(index, (index + count));
    client.write((byte*)substr.c_str(), count);
    index += count;
  }
  client.endPublish();
}

void cover_cmd_subscriber(String topic, String message)
{
  if (message == "OPEN")
  {
    door.trigger_action(hoermann_action_open);
  }
  else if (message == "CLOSE")
  {
    door.trigger_action(hoermann_action_close);
  }
  else if (message == "STOP")
  {
    door.trigger_action(hoermann_action_stop);
  }
}

void venting_cmd_subscriber(String topic, String message)
{
  if (message == "ON")
  {
    door.trigger_action(hoermann_action_venting);
  }
  else if (message == "OFF")
  {
    door.trigger_action(hoermann_action_close);
  }
}

void light_cmd_subscriber(String topic, String message)
{
  if ((message == "ON") || (message == "OFF"))
  {
    door.trigger_action(hoermann_action_toggle_light);
  }
}
