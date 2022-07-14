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

char* ssid = "BELL4G-CB08"; //"NOSSID";
char* password = "31900savitha"; //"NOPASSWORD";
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

int Buzzer=9;
int Emergency=3;

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
  digitalWrite(Buzzer, HIGH); delay(10); digitalWrite(Buzzer, LOW); delay(10);
  digitalWrite(Buzzer, HIGH); delay(10); digitalWrite(Buzzer, LOW); delay(10);
  digitalWrite(Buzzer, HIGH); delay(10); digitalWrite(Buzzer, LOW); delay(10);
}

PubSubClient client(AWS_endpoint, 8883, callback, espClient);

/*void addUser() {
  char* user;
  char* email;
  char* webpwd;
  while (!Serial.available());
  strcpy(user, Serial.readString().c_str());
  while (!Serial.available());
  strcpy(email, Serial.readString().c_str());
  while (!Serial.available());
  strcpy(webpwd, Serial.readString().c_str());
  if (needWiFi) Serial.write("NoWiFi");
  else {
    snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"UserReg\", \"user\": \"%s\", \"email\": \"%s\", \"password\": \"%s\"}", deviceId, user, email, webpwd);
    client.publish("outTopic", msg);
    delay(6000);
    strcpy(username, user); Serial.write("UserRegistered");
  }
}*/

bool initWifi() {                                                                            // Reinitializing WiFi connectivity after awaking
  WiFi.begin(ssid, password); delay(500);
  int count=0;
  while((count<1000) && (WiFi.status()!=WL_CONNECTED)){digitalWrite(16,HIGH);delay(25);digitalWrite(16, LOW);delay(25);count++;} 
  delay(500);
  return WiFi.status()==WL_CONNECTED ? true : false;
 }

bool initTime() {
  timeClient.begin();
  int count=0;
  while ((count<1000) && (!timeClient.update())) {timeClient.forceUpdate(); digitalWrite(16,HIGH);delay(25);digitalWrite(16, LOW);delay(25);count++;}
  if (count<1000) {espClient.setX509Time(timeClient.getEpochTime()); return true;}
  else return false;
}

/*void getWiFi(){
  while (!Serial.available());
  strcpy(ssid, Serial.readString().c_str());
  while (!Serial.available());
  strcpy(password, Serial.readString().c_str());
  digitalWrite(16, LOW);
  if (initWifi()) {digitalWrite(16, HIGH); 
    if (initTime()) {digitalWrite(16, HIGH); Serial.write("Success"); needWiFi=false;}
    else {digitalWrite(16, HIGH); Serial.write("TIme error"); needWiFi=true;}
    }
  else {digitalWrite(16, HIGH); Serial.write("Error"); needWiFi=true;}
  delay(2000);
}

void handleSerial(){
  delay(10); String command = Serial.readString();
  if (command=="getUser") Serial.write(username);
  else if (command=="addUser") addUser();
  else if (command=="setWiFi") getWiFi();
  else Serial.write("Unknown command");
}*/

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
long NOW=0;
long EM_now=0;
long last_Em_time=0;

bool check_emergency(){
  NOW=millis();
  if (digitalRead(Emergency)==true && (NOW-last_Em_time)>10000){
    EM_now = millis();
    if ((EM_now - lastMsg > 2000) && (digitalRead(Emergency)==true)){
        lastMsg = EM_now;
        last_Em_time=millis();
        digitalWrite(Buzzer,HIGH); delay(100); digitalWrite(Buzzer,LOW);
        Status="Emergency";
        snprintf(msg,256, "{\"dev\": \"%s\", \"t\": \"Emergency\"}", deviceId);
        client.publish("outTopic", msg);
      }
  }
  else if (digitalRead(Emergency)==true && (NOW-last_Em_time)<10000){
    Status="Normal";
    digitalWrite(Buzzer,HIGH); delay(25); digitalWrite(Buzzer,LOW); delay(25);
    digitalWrite(Buzzer,HIGH); delay(25); digitalWrite(Buzzer,LOW); delay(25);
    snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Normal\"}", deviceId);
    client.publish("outTopic", msg);
  }
}  

void setup() {
  pinMode(2, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(3, INPUT);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(16, HIGH);
  digitalWrite(Buzzer, LOW);
//  Serial.begin(115200);
//  Serial.setTimeout(100);
  setupAWS();
  while (!mpu.begin()){
    digitalWrite(2,LOW);
    delay(250);
    digitalWrite(2,HIGH);
    delay(250);
  }
  digitalWrite(2, HIGH);
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  initWifi();
  initTime();
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
}

void loop() {

  digitalWrite(12, digitalRead(3));

  if (!needWiFi) {
    digitalWrite(2, LOW);
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
        digitalWrite(Buzzer, HIGH); delay(100); digitalWrite(Buzzer, LOW);
        client.publish("outTopic", msg);
        int cnt_emg = 0;
        while (cnt_emg<20){
          digitalWrite(Buzzer,HIGH); delay(250);
          digitalWrite(Buzzer,LOW); delay(250);
          cnt_emg++;
        }
        digitalWrite(Buzzer, HIGH); delay(100); digitalWrite(Buzzer, LOW);
        client.publish("outTopic", msg);
        Status="Fallen confirmed";
      }
      
    }
    // ######################################################################################
//    check_emergency();

    if (New_Message){
      if (Type=="Medication"){
        Medic=MSG;
        int cnt_medic = 0;
        while (cnt_medic<60 or digitalRead(Emergency)==false){
          digitalWrite(Buzzer,HIGH); delay(50); digitalWrite(Buzzer,LOW); delay(50);
          digitalWrite(Buzzer,HIGH); delay(50); digitalWrite(Buzzer,LOW); delay(350);
          cnt_medic++;
        }
        if (cnt_medic==60) {
          snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Medication misseed\"}", deviceId);
        }
        New_Message=false;
        digitalWrite(Buzzer,LOW);
        }

      else if (Type=="Warning"){
        digitalWrite(12, HIGH);
        int cnt_emg = 0;
        while (cnt_emg<20 or digitalRead(Emergency)==true){
          digitalWrite(Buzzer,HIGH); delay(250);
          digitalWrite(Buzzer,LOW); delay(250);
          cnt_emg++;
        }
        digitalWrite(Buzzer,LOW);
        if (cnt_emg==20) {
          Status="Fallen confirmed";
          snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Fallen\"}", deviceId);
        } else {
          Status="Normal";
          snprintf(msg, 256, "{\"dev\": \"%s\", \"t\": \"Normal\"}", deviceId);
        }
        client.publish("outTopic", msg);
        New_Message=false;
        digitalWrite(12, LOW);
        }
    }
    digitalWrite(16, HIGH); delay(100); digitalWrite(16, LOW); delay(100);
    if (Status=="Fallen confirmed") {
      digitalWrite(Buzzer,HIGH); delay(900);
      digitalWrite(Buzzer,LOW); delay(100);
    }
  }
  else digitalWrite(2, HIGH);
  // Fallen confirmed
}
