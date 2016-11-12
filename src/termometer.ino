#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Streaming.h"

#define TEMP_SENSOR D4

const int ID = ESP.getChipId();                 // Device ID
const char* WIFI_SSID = "<WIFI>";               // WiFi Name
const char* WIFI_PASSWORD = "<PASSWORD>";       // WiFi Password
const char* MQTT_HOST = "<IP OR HOST>";         // MQTT Broker Host
const int   MQTT_PORT = 1883;                   // MQTT Port
const char* MQTT_USER = "<USER>";               // MQTT User
const char* MQTT_PASSWORD = "<PASSWORD>";       // MQTT Password
const char* MQTT_TOPIC = "/sensor/";            // MQTT Topic
const int   INTERVAL = 5000;                    // How often ESP8266 should get temperature value from the sensor
const float TEMP_CORRECTION = 0;                // Temperatur correction
const int   CONNECTION_WAIT_TIME = 5000;        // How long ESP8266 should wait before next attempt to connect to WiFi or MQTT Broker

char  mqttTopic[35];
float memorizedTemperature = 0;
char  temperatureString[6];

WiFiClient esp;
PubSubClient client(esp);
OneWire wireProtocol(TEMP_SENSOR);
DallasTemperature DS18B20(&wireProtocol);

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  Serial << " MQTT Broker: " << MQTT_HOST << ":" << MQTT_PORT << endl;
  sprintf(mqttTopic, "%s%i/temperature", MQTT_TOPIC, ID);
  Serial << " Topic: " << mqttTopic << endl;
  client.setServer(MQTT_HOST, MQTT_PORT);
  DS18B20.begin();
  Serial << "-----------------------------------------------------" << endl;
}

void connectToWiFi() {
  Serial.println();
  Serial << "Connecting to " << WIFI_SSID;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(CONNECTION_WAIT_TIME);
    Serial << ".";
  }

  Serial.println();
  Serial << "-----------------------------------------------------" << endl;
  Serial << " Device ID: " << ID << endl;
  Serial << " WiFi connected: " << WIFI_SSID << endl;
  Serial << " IP address: " << WiFi.localIP()  << endl;
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial << "Connecting to MQTT Broker: " << MQTT_HOST << ":" << MQTT_PORT << endl;
    if (client.connect("ESP8266 Sensor", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println();
    } else {
      Serial << "ESP8266 will try to connected in : " << CONNECTION_WAIT_TIME/1000 << "sec" << endl;;
      delay(CONNECTION_WAIT_TIME);
    }
  }
}

float getTemperature() {
  float temperature;
  do {
    DS18B20.requestTemperatures();
    temperature = DS18B20.getTempCByIndex(0);
  } while (temperature == 85.0 || temperature == (-127.0));
  return temperature + TEMP_CORRECTION;
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
  float temperature = getTemperature();
  if (memorizedTemperature!=temperature) {
      memorizedTemperature = temperature;
      dtostrf(temperature, 2, 1, temperatureString);
      Serial << "Publising: " << temperatureString << endl;
      client.publish(mqttTopic, temperatureString);
  }
  delay(INTERVAL);
}