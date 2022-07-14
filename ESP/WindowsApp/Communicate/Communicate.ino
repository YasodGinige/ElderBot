#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

char* ssid = "NOSSID";
char* password = "NOPASSWORD";
bool needWiFi = true;
char* username = "UNREGISTERED";
char* deviceId = "Pamu_ESP";

void addUser() {
  while (!Serial.available());
  String user = Serial.readString();
  while (!Serial.available());
  String email = Serial.readString();
  while (!Serial.available());
  String webpwd = Serial.readString();
  if (needWiFi) Serial.write("NoWiFi");
  else {
    // Send unique key, username, email and password to cloud
    strcpy(username, user.c_str());
    Serial.write("UserRegistered");
  }
}

bool initWifi() {                                                                            // Reinitializing WiFi connectivity after awaking
  WiFi.begin(ssid, password); delay(500);
  int count=0;
  while((count<1000) && (WiFi.status()!=WL_CONNECTED)){digitalWrite(16,HIGH);delay(25);digitalWrite(16, LOW);delay(25);count++;} 
  delay(500);
  return WiFi.status()==WL_CONNECTED ? true : false;
 }

void getWiFi(){
  while (!Serial.available());
  strcpy(ssid, Serial.readString().c_str());
  while (!Serial.available());
  strcpy(password, Serial.readString().c_str());
  digitalWrite(16, LOW);
  if (initWifi()) {digitalWrite(16, HIGH); Serial.write("Success"); needWiFi=false;}
  else {digitalWrite(16, HIGH); Serial.write("Error"); needWiFi=true;}
  delay(2000);
}

void handleSerial(){
  delay(10); String command = Serial.readString();
  if (command=="getUser") Serial.write(username);
  else if (command=="addUser") addUser();
  else if (command=="setWiFi") getWiFi();
  else Serial.write("Unknown command");
}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(16, OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(16, HIGH);
  Serial.begin(115200);
  Serial.setTimeout(100);
}

void loop() {
  if (Serial.available()) handleSerial();
  else if (needWiFi) digitalWrite(2, HIGH);
  else digitalWrite(2, LOW);
}
