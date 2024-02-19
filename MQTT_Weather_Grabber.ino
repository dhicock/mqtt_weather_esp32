#include <stdio.h>
#include <WiFi.h>
#include <PubSubClient.h>

//WiFi
const char *ssid = "***";
const char *wifipwd = "***";

//MQTT
const char *mqtt_broker = "10.0.0.223";
const char *mqtt_username = "mosquitto";
const char *mqtt_password = "q2m0veFL47uAzDvbt57h5s0X";
const int mqtt_port = 1883;
const char *topics[] = {"weather/outTemp_F", "weather/windSpeed_mph", "weather/heatindex_F", "weather/outHumidity", "weather/windDir", "weather/rainRate_inch_per_hour", "weather/rain_in", "weather/hourRain_in", "weather/dayRain_in"};

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Set software serial baud to 115200;
    Serial.begin(115200);

    Serial.println();
    Serial.println("#################");
    Serial.println("## NEW SESSION ##");
    Serial.println("#################");
    Serial.println();
    // Connecting to a WiFi network
    WiFi.begin(ssid, wifipwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the Wi-Fi network");

    //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    client.setBufferSize(4096);
    while (!client.connected()) {
        String client_id = "esp32-client";
        Serial.printf("The client %s connects to the MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("The MQTT broker is connected");
            Serial.println();
      Serial.println();
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    // Subscribe to topic
    int topicCount = sizeof(topics) / sizeof(*topics);
    Serial.printf("Subscribing to %d topics.", topicCount);
    Serial.println();

    for(int i = 0; i < topicCount; i++) {
      const char *topic = topics[i];
      if(client.subscribe(topic)){
        Serial.print("Subscribed to ");
        Serial.print(+topic);
        Serial.println();
      }
    }
    
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
}

void loop() {
  client.loop();
  delay(1000);
}