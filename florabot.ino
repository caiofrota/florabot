#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// WiFi
const char* ssid = "SSID";
const char* password = "PASSWORD";
WiFiClient espClient;

// Home Assistant Credentials
const char* haUser = "mqtt-user";
const char* haPassword = "mqtt-password";

// MQTT Network
const char* mqttServer = "mqtt-server";
const uint16_t mqttPort = 1883;
const char* mqttDeviceId = "plant_1";
const char* mqttMoistureTopic = "home/plants/1/moisture";
const char* mqttBatteryTopic = "home/plants/1/battery";
const char* mqttHumidityTopic = "home/plants/1/humidity";
const char* mqttTemperatureTopic = "home/plants/1/temperature";
PubSubClient client(espClient);

// DHT
//DHT dht(D5, DHT11);
DHT dht(D5, DHT22);

// Mux
const int S0 = D6;
const int S1 = D7;
const int S2 = D8;

// Run =========================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  setup_wifi();

  if(WiFi.status() == WL_CONNECTED && connect_mqtt()) {
    setupMux(S0, S1, S2);
    dht.begin();
    
    int moisture = readMoisture();
    int battery = readBattery();
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    char buffer[16];
    
    // Moisture
    sprintf(buffer, "%d", moisture);
    publishMqtt(mqttMoistureTopic, buffer);
    
    // Battery
    sprintf(buffer, "%d", battery);
    publishMqtt(mqttBatteryTopic, buffer);
    
    // Humidity
    sprintf(buffer, "%.2f", humidity);
    publishMqtt(mqttHumidityTopic, buffer);
    
    // Temperature
    sprintf(buffer, "%.1f", temperature);
    publishMqtt(mqttTemperatureTopic, buffer);
    
    client.disconnect();
    Serial.println("All set! Next reading in 2 hours. Bye...");
    
    delay(100);
    ESP.deepSleep(2 * 60 * 60e6);
  } else {
    Serial.println("Connection failed! Trying again in 30 minutes.");
    ESP.deepSleep(30 * 60e6);
  }
}

void loop() {}

// Readings ====================================================================

int readMoisture() {
  return readMuxChannel(0);
}

int readBattery() {
  float battery = readMuxChannel(1) / 1023.0 * 3.15 * 5.0;
  return round((battery - 3.0) / (4.2 - 3.0) * 100.0);
}

// MQTT ========================================================================

bool connect_mqtt() {
  client.setServer(mqttServer, mqttPort);
  Serial.print("Connecting to MQTT...");
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    if (client.connect(mqttDeviceId, haUser, haPassword)) {
      Serial.println("Conected!");
      return true;
    } else {
      Serial.print(".");
      delay(1000);
      attempts++;
    }
  }
  Serial.println("Fail!");
  return false;
}

void publishMqtt(const char* topic, const char* payload) {
  if (!client.connected()) {
    Serial.println(String(topic) + " => MQTT not connected");
    return;
  }
  
  if (payload == nullptr || strlen(payload) == 0) {
    Serial.println(String(topic) + " => Null or empty payload");
    return;
  }

  client.publish(topic, payload, true);
  Serial.println(String(topic) + " => " + payload);
}

// WiFi ========================================================================

void setup_wifi() {
  Serial.print("\nConnecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 30000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// MUX =========================================================================

void setupMux(int pin1, int pin2, int pin3) {
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);
  pinMode(pin3, OUTPUT);
}

void setMuxChannel(int ch) {
  digitalWrite(S0, bitRead(ch, 0));
  digitalWrite(S1, bitRead(ch, 1));
  digitalWrite(S2, bitRead(ch, 2));
  delay(500);
}

int readMuxChannel(int ch) {
  setMuxChannel(ch);
  return analogRead(A0);
}
