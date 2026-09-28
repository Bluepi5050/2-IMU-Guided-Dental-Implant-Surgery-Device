// This code is for the ESP8266 module connected to the MPU6050 sensor
// Receives the BNO angle data from the webserver and executes calculation using the MPU data and received BNO data
// Representing the sensor attached to the handpiece


// The sensors can be the same module (Both BNO can be better)

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <MPU6050.h>
#include <math.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Galaxy S24+ EB72";
const char* password = "";

MPU6050 mpu;

float filteredPitch = 0.0;
float filteredRoll  = 0.0;

unsigned long prevTime = 0;

String sensorModuleHost = "http://bno.local/"; 
const int sensorPort = 80;

const int LED_PIN = D5;

const int photo = A0;

int threshold = 100;

int isPressed = 0;

float rollDiff, pitchDiff = 0.0;
float b_rollDiff, b_pitchDiff = 0.0;

float bnoRoll, bnoPitch = 0.0;


void setup() {                   // setup the wifi connection
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  if(MDNS.begin("led")){
    Serial.println("LED module mDNS started");
  }

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed. Check wiring!");
    while (1);
  }
  Serial.println("MPU6050 connected successfully");

  prevTime = millis();
}

void loop() {

  unsigned long currTime = millis();
  float dt = (currTime - prevTime) / 1000.0;
  prevTime = currTime;

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  float accPitch = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 180.0 / PI;
  float accRoll  = atan2(ay_g, az_g) * 180.0 / PI;

  float gyroXrate = gx / 131.0;
  float gyroYrate = gy / 131.0;

  filteredPitch = 0.96 * (filteredPitch + gyroYrate * dt) + 0.04 * accPitch;                   // getting data from the MPU sensor
  filteredRoll  = 0.96 * (filteredRoll  + gyroXrate * dt) + 0.04 * accRoll;

  if(WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    String url = sensorModuleHost;

    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {                 // get the orientation data from the webserver (just roll and pitch, yaw isn't needed)

      String payload = http.getString();      
      
      int zIndex   = payload.indexOf("Z:");
      if(zIndex != -1) {
        int newlineIndex = payload.indexOf("\n", zIndex);
        String zStr;
        if(newlineIndex == -1) {
          zStr = payload.substring(zIndex + 2);
        } else {
          zStr = payload.substring(zIndex + 2, newlineIndex);
        }
        zStr.trim();
        bnoPitch = zStr.toFloat() * -1;
      }

      int yIndex   = payload.indexOf("Y:");
      if(yIndex != -1) {
        int newlineIndex_Y = payload.indexOf("\n", yIndex);
        String yStr;
        if(newlineIndex_Y == -1) {
          yStr = payload.substring(yIndex + 2);
        } else {
          yStr = payload.substring(yIndex + 2, newlineIndex_Y);
        }
        yStr.trim();
        bnoRoll = yStr.toFloat();
      }

      rollDiff = filteredRoll - bnoRoll;                       // the difference in orientations of two sensors
      pitchDiff = filteredPitch - bnoPitch;

      if (analogRead(photo) > threshold && isPressed == 0) {                // if the switch is on for the first time then save the differences as the benchmark

        isPressed = 1;
        b_rollDiff = rollDiff;
        b_pitchDiff = pitchDiff;

        delay(100);

      } else if (isPressed == 1) {                        // after the switch is on, compare the differences in angles and see if it goes over the threshold (here it is 5 degrees as an example)

        if (abs(rollDiff - b_rollDiff) > 5.00 || abs(pitchDiff - b_pitchDiff) > 5.00) {
          digitalWrite(LED_PIN, HIGH);
        } else {
          digitalWrite(LED_PIN, LOW);
        }

      }

    } else {
      Serial.print("HTTP error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
  
  delay(200);

}
