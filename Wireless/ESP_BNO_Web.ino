// This code is for the ESP8266 module connected to the BNO055 sensor
// Representing the sensor attached to the adjacent tooth
// Sends sensor data to a web server accessible through another ESP module


#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

const char* ssid = "Galaxy S24+ EB72";           // ESP8266 connects to this wifi
const char* password = "";

ESP8266WebServer server(80);

Adafruit_BNO055 bno = Adafruit_BNO055(55);

void handleSensorData() {                   // get the orientation data from the BNO sensor and send the data to a web server by ESP8266
  sensors_event_t event;
  bno.getEvent(&event);
  
  String sensorData = "X: " + String(event.orientation.x, 2) + "\n" +
                      "Y: " + String(event.orientation.y, 2) + "\n" +
                      "Z: " + String(event.orientation.z, 2);
  
  server.send(200, "text/plain", sensorData);
  Serial.println("Sending sensor data:");
  Serial.println(sensorData);
}

void setup() {                            // setting up ESP web server
  Serial.begin(115200);
  Serial.println("Starting BNO055 sensor module...");

  if (!bno.begin()) {
    Serial.println("BNO055 not detected! Check wiring.");
    while (1);
  }
  delay(1000);
  bno.setExtCrystalUse(true);

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  if(MDNS.begin("bno")){
    Serial.println("mDNS responder started: bno.local");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", handleSensorData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  MDNS.update();
}