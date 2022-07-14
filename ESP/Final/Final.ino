
#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

char msg[256];

Adafruit_MPU6050 mpu;
sensors_event_t a,g,temp;

char* ssid = "vivo1727"; //"NOSSID";
char* password = "12345678"; //"NOPASSWORD";
bool needWiFi = false; //true
char* username = "TestUser";
char* deviceId = "Pamu_ESP"; // Must be 8 letters

String deviceId_Per="Pamu_ESP";
String Type="None";
String MSG="None";
bool New_Message=false;
char* Status="Normal";
String Resp_UserApp="";
String Medic="";

int buzzer = 9;
int button = 3;

int wifi_LED = 12;
int status_LED = 13;
int emergency_LED = 2;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClientSecure espClient;

const char* AWS_endpoint = "a3diuj9r5yc78a-ats.iot.us-east-2.amazonaws.com";

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void callback(char* topic, byte* payload, unsigned int Length) {                     // Take the incomming MQTT Messages and update NodeMCU global variables
  String Data_in = "";
  for (int i = 1; i < Length-1; i++) {
    Data_in += (char)payload[i];
  }
  deviceId_Per = getValue(Data_in, ',', 0);
  Type = getValue(Data_in, ',', 1);
  MSG = getValue(Data_in, ',', 2);
  New_Message = true;
  digitalWrite(buzzer, HIGH); delay(10); digitalWrite(buzzer, LOW); delay(10);
  digitalWrite(buzzer, HIGH); delay(10); digitalWrite(buzzer, LOW); delay(10);
  digitalWrite(buzzer, HIGH); delay(10); digitalWrite(buzzer, LOW); delay(10);
}

PubSubClient client(AWS_endpoint, 8883, callback, espClient);

bool initWifi() {                                                                            // Reinitializing WiFi connectivity after awaking
  WiFi.begin(ssid, password); delay(500);
  int count=0;
  while((count<1000) && (WiFi.status()!=WL_CONNECTED)){digitalWrite(wifi_LED,HIGH);delay(25);digitalWrite(wifi_LED, LOW);delay(25);count++;} 
  delay(500);
  return WiFi.status()==WL_CONNECTED ? true : false;
 }

bool initTime() {
  timeClient.begin();
  int count=0;
  while ((count<1000) && (!timeClient.update())) {timeClient.forceUpdate(); digitalWrite(wifi_LED,HIGH);delay(25);digitalWrite(wifi_LED, LOW);delay(25);count++;}
  if (count<1000) {espClient.setX509Time(timeClient.getEpochTime()); return true;}
  else return false;
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(deviceId)) {
      snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Connection Established\"}", deviceId);
      client.publish("outTopic", msg);
      client.subscribe("inTopic");
    } else {
      delay(5000);
    }
  }
}

bool setupAWS(){
  delay(1000);
  if (!SPIFFS.begin()) {return false;}

  File cert = SPIFFS.open("/cert.der", "r");
  if (!cert) return false;
  delay(1000);
  if (!espClient.loadCertificate(cert)) return false;

  File private_key = SPIFFS.open("/private.der", "r");
  if (!private_key) return false;
  delay(1000);
  if (!espClient.loadPrivateKey(private_key)) return false;

  File ca = SPIFFS.open("/ca.der", "r");
  if (!ca) return false;
  delay(1000);
  if (!espClient.loadCACert(ca)) return false;

  return true;
}

float acclx = 0;
float accly = 0;
float acclz = 0;
float gyrox = 0;
float gyroy = 0;
float gyroz = 0;

float alpha = 0.95;
float accPrev = 0;
float aXPrev = 0;
float aYPrev = 0;
float aZPrev = 0;
float gXPrev = 0;
float gYPrev = 0;
float gZPrev = 0;

float accThresh = 5;
float gXThresh = 1.5;
float gYThresh = 1.5;
float gZThresh = 1.5;
float gMin = 0.1;

bool makeDecision(float aX, float aY, float aZ, float gX, float gY, float gZ){
  // calibrate readings
  aX = alpha * aX + (1 - alpha) * aXPrev;
  aY = alpha * aY + (1 - alpha) * aYPrev;
  aZ = alpha * aZ + (1 - alpha) * aZPrev;

  // threshold to avoid random changes
  if (abs(gX-gXPrev) > gMin){
    gX = alpha * gX + (1 - alpha) * gXPrev;
  } else{
    gX = gXPrev;
  }
  if (abs(gY-gYPrev) > gMin){
    gY = alpha * gY + (1 - alpha) * gYPrev;
  } else{
    gY = gYPrev;
  }
  if (abs(gZ-gZPrev) > gMin){
    gZ = alpha * gZ + (1 - alpha) * gZPrev;
  } else{
    gZ = gZPrev;
  }
  // Calculate acceleration
  float acc = sqrt(sq(aX) + sq(aY) + sq(aZ));
  float accChange = abs(acc - accPrev);
  accPrev = acc;

  // calculate gyro change
  float gXChange = abs(gX - gXPrev);
  float gYChange = abs(gY - gYPrev);
  float gZChange = abs(gZ - gZPrev);

  // decision
  bool decision = false;
  if (accChange > accThresh){
    if (gXChange > gXThresh){
      decision = true;
    } 
    else if(gYChange > gYThresh){
      decision = true;
    }
    else if (gZChange > gZThresh){
      decision = true;
    }
  }

  aXPrev = aX; aYPrev = aY; aZPrev = aZ; gXPrev = gX; gYPrev = gY; gZPrev = gZ;
  
  return decision;
}

long lastMsg = 0;

bool check_emergency(){
  if (digitalRead(button)==false) {
    delay(2000);
    if (digitalRead(button)==false){
      digitalWrite(buzzer,HIGH); delay(100); digitalWrite(buzzer,LOW);
      Status="Emergency";
      snprintf(msg,256, "{\"dev\": \"%s\", \"t\": \"Fallen\"}", deviceId);
      client.publish("outTopic", msg);
    }
  }
}  

void setup() {
  pinMode(status_LED, OUTPUT);
  pinMode(wifi_LED, OUTPUT);
  pinMode(emergency_LED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  digitalWrite(status_LED, LOW);
  digitalWrite(wifi_LED, LOW);
  digitalWrite(emergency_LED, LOW);
  digitalWrite(buzzer, LOW);
  setupAWS();
  while (!mpu.begin()){
    digitalWrite(status_LED,LOW);
    delay(250);
    digitalWrite(status_LED,HIGH);
    delay(250);
  }
  digitalWrite(status_LED, HIGH);
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  initWifi();
  initTime();
}

long fallTime = 0;

void loop() {
  if (!needWiFi) {
    digitalWrite(status_LED, HIGH);
    if (!client.connected()) reconnect();
    client.loop();

    long now = millis();
    if (now - lastMsg > 250 and Status=="Normal"){
      lastMsg = now;
      mpu.getEvent(&a, &g, &temp);
      acclx = a.acceleration.x;
      accly = a.acceleration.y;
      acclz = a.acceleration.z;
      gyrox = g.gyro.x;
      gyroy = g.gyro.y;
      gyroz = g.gyro.z;
      bool decision = makeDecision(acclx, accly, acclz, gyrox, gyroy, gyroz);
      if (decision) {
        snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Fallen\"}", deviceId);
        digitalWrite(buzzer, HIGH); delay(100); digitalWrite(buzzer, LOW);
        client.publish("outTopic", msg);
        Status="Fallen"; digitalWrite(emergency_LED, HIGH);
        fallTime = millis();
      }
    }

    if (Status=="Fallen confirmed" or Status=="Emergency") {
      digitalWrite(emergency_LED, HIGH);
      digitalWrite(buzzer,HIGH); delay(900);
      digitalWrite(buzzer,LOW); delay(100);
    } else if (Status=="Fallen") {
      if ((millis()-fallTime) > 10000){
        Status="Fallen confirmed";
        snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Fallen\"}", deviceId);
        digitalWrite(buzzer, HIGH); delay(100); digitalWrite(buzzer, LOW);
        client.publish("outTopic", msg);
      }
      else if (digitalRead(button)==false) {
        delay(2000);
        if (digitalRead(button)==false){
          snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Normal\"}", deviceId);
          client.publish("outTopic", msg);
          Status="Normal"; digitalWrite(emergency_LED, LOW); fallTime=millis();
        }
      }
      digitalWrite(emergency_LED, HIGH);
      digitalWrite(buzzer,HIGH); delay(100);
      digitalWrite(buzzer,LOW); delay(100);
    } else if (Status=="Normal") {
      check_emergency(); digitalWrite(emergency_LED, LOW);
    }

    if (New_Message){
      if (Type=="Medication"){
        Medic=MSG;
        int cnt_medic = 0;
        while (cnt_medic<60 or digitalRead(button)==true){
          digitalWrite(buzzer,HIGH); delay(50); digitalWrite(buzzer,LOW); delay(50);
          digitalWrite(buzzer,HIGH); delay(50); digitalWrite(buzzer,LOW); delay(350);
          cnt_medic++;
        }
        if (cnt_medic==60) {
          snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Medication misseed\"}", deviceId);
          client.publish("outTopic", msg);
        }
        New_Message=false;
        digitalWrite(buzzer,LOW);
        }

      else if (Type=="Warning"){
        New_Message=false;
       }
    }
    digitalWrite(emergency_LED, HIGH); delay(100); digitalWrite(emergency_LED, LOW);
  }
  
  else digitalWrite(status_LED, LOW);
}
