#include <stdio.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "arduino_secrets.h"
#include "time.h"

//Time Server
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

//WiFi
const char *ssid = SECRET_SSID;
const char *wifipwd = SECRET_PASS;

//MQTT
const char *mqtt_broker = SECRET_MQTT_URL;
const char *mqtt_username = SECRET_MQTT_USER;
const char *mqtt_password = SECRET_MQTT_PASS;
const int mqtt_port = SECRET_MQTT_PORT;
const char *topics[] = { "weather/outTemp_F", "weather/windSpeed_mph", "weather/heatindex_F", "weather/outHumidity", "weather/windDir", "weather/rainRate_inch_per_hour", "weather/rain_in", "weather/hourRain_in", "weather/dayRain_in", "weather/dateTime" };

//Google Sheets
String GOOGLE_SCRIPT_ID = SECRET_GOOGLE_SCRIPT;

//Globals
WiFiClient espClient;
PubSubClient client(espClient);

//Structs
struct payloadData {
  float outTemp;
  float windSpeed;
  float outHeatIndex;
  float outHumidity;
  float windDir;
  float rainRate_perHour;
  float rain_in;
  float rain_hour_in;
  float rain_day_in;
  long timestamp;
};

payloadData payloadPacket;
bool data_outTemp, data_windSpeed, data_heatindex, data_outHumidity, data_windDr, data_rain_per_hour, data_rain_in, data_hourRain, data_dayRain, data_timestamp = false;

void setup() {
  // Set software serial baud to 115200;
  delay(1000);
  Serial.begin(115200);
  delay(1000);

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
  delay(1000);
  Serial.flush();

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
      Serial.flush();
      Serial.println();
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      Serial.println();
      Serial.println();
      delay(2000);
    }
  }
  // Subscribe to topic
  int topicCount = sizeof(topics) / sizeof(*topics);
  Serial.printf("Subscribing to %d topics.", topicCount);
  Serial.println();

  for (int i = 0; i < topicCount; i++) {
    const char *topic = topics[i];
    if (client.subscribe(topic)) {
      Serial.print("Subscribed to ");
      Serial.print(+topic);
      Serial.println();
    }
  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char data[length];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data[i] = (char)payload[i];
  }
  Serial.println();
  bool submitToGoogle = addData(topic, data);
  if (submitToGoogle) {
    bool uploadSuccessful = uploadToGoogle(encodeData());
    if(uploadSuccessful){
      Serial.println("Successfully uploaded to Google");
      Serial.flush();
    }
  }
  delay(1000);
}

bool addData(char *topic, char *data) {
  if (strcmp(topic, "weather/outTemp_F") == 0) {
    payloadPacket.outTemp = atof(data);
    data_outTemp = true;
  }
  if (strcmp(topic, "weather/windSpeed_mph") == 0) {
    payloadPacket.windSpeed = atof(data);
    data_windSpeed = true;
  }
  if (strcmp(topic, "weather/heatindex_F") == 0) {
    payloadPacket.outHeatIndex = atof(data);
    data_heatindex = true;
  }
  if (strcmp(topic, "weather/outHumidity") == 0) {
    payloadPacket.outHumidity = atof(data);
    data_outHumidity = true;
  }
  if (strcmp(topic, "weather/windDir") == 0) {
    payloadPacket.windDir = atof(data);
    data_windDr = true;
  }
  if (strcmp(topic, "weather/rainRate_inch_per_hour") == 0) {
    payloadPacket.rainRate_perHour = atof(data);
    data_rain_per_hour = true;
  }
  if (strcmp(topic, "weather/rain_in") == 0) {
    payloadPacket.rain_in = atof(data);
    data_rain_in = true;
  }
  if (strcmp(topic, "weather/hourRain_in") == 0) {
    payloadPacket.rain_hour_in = atof(data);
    data_hourRain = true;
  }
  if (strcmp(topic, "weather/dayRain_in") == 0) {
    payloadPacket.rain_day_in = atof(data);
    data_dayRain = true;
  }
  if (strcmp(topic, "weather/dateTime") == 0) {
    payloadPacket.timestamp = atoi(data);
    data_timestamp = true;
  }
  if (data_outTemp && data_windSpeed && data_heatindex && data_outHumidity && data_windDr && data_windSpeed && data_rain_per_hour && data_rain_in && data_hourRain && data_dayRain && data_timestamp) {
    Serial.println("I have all my data! Sending to Google.");
    data_outTemp = data_windSpeed = data_heatindex = data_outHumidity = data_windDr = data_windSpeed = data_rain_per_hour = data_rain_in = data_hourRain = data_dayRain = data_timestamp = false;
    return true;
  }
  return false;
}

String encodeData() {
  String encodedData = "&outsideTemp=" + String(payloadPacket.outTemp)
                       + "&heatIndex=" + String(payloadPacket.outHeatIndex)
                       + "&outsideHumidity=" + String(payloadPacket.outHumidity)
                       + "&windSpeed=" + String(payloadPacket.windSpeed)
                       + "&windDir=" + String(payloadPacket.windDir)
                       + "&rainRate=" + String(payloadPacket.rainRate_perHour)
                       + "&rainTotal=" + String(payloadPacket.rain_in)
                       + "&rainTotalHour=" + String(payloadPacket.rain_hour_in)
                       + "&rainTotalDay=" + String(payloadPacket.rain_day_in)
                       + "&timestamp=" + String(payloadPacket.timestamp);
  return encodedData;
}

bool uploadToGoogle(String encodedData) {
  if (WiFi.status() == WL_CONNECTED) {
    static bool flag = false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return false;
    }
    char timeStringBuff[50];  //50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T %Z", &timeinfo);
    String asString(timeStringBuff);
    asString.replace(" ", "%20");
    Serial.print("Time:");
    Serial.println(asString);
    String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "date=" + asString + encodedData;
    Serial.print("POST data to spreadsheet:");
    Serial.println(urlFinal);
    HTTPClient http;
    http.begin(urlFinal.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    if (httpCode > 0) {
      Serial.println("Success!");
    } else {
      Serial.println("Failure.");
    }
    if (http.connected()) {
      Serial.println("HTTP Connected.");
      http.end();
      Serial.println("HTTP Closed");
    }
    Serial.println("Returning to main function");
    Serial.println();
    return true;
  }
}

void loop() {
  client.loop();
}