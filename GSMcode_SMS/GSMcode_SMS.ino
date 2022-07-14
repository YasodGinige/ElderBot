#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <SoftwareSerial.h>

char* ssid = "SLT-ADSL-BAC21";
char* password = "gl2244652##";
//char* UserInfo ="";
bool needWiFi = true;
char str[32] = { 0 };
String Data;
const char* AWS_endpoint = "a3diuj9r5yc78a-ats.iot.us-east-2.amazonaws.com";
long lastMsg = 0;
char msg[50];
char* Sens_Read="0";
char* Status="Normal";             // States: Normal / Fallen / Emergency
int value=0;


const char* D2S = "D2S";
const char* S2D = "S2D";

String Device_ID_Per="Yasod_ESP";
String Device_ID="0";
String Phone_Num="None";
String MSG="None";
bool New_Message=false;

String Resp_UserApp="";
String Medic="";


int Buzzer=6;
uint8_t Emergency = D3;

SoftwareSerial Sim800l(2, 3); // RX,TX for Arduino and for the module it's TXD RXD, they should be inverted


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient);

void callback(char* topic, byte* payload, unsigned int Length) {                     // Take the incomming MQTT Messages and update NodeMCU global variables
  String Data_in = "";
  for (int i = 0; i < Length; i++) {
    Data_in += (char)payload[i];
  }
    Device_ID = getValue(Data_in, ',', 0);
    Phone_Num = getValue(Data_in, ',', 1);
    MSG = getValue(Data_in, ',', 2);
  
    char msgr[30]="MQTT receievd";
    Serial.println(msgr);
    Serial.print(Phone_Num);
    Serial.println(MSG);
    New_Message=true;
    
}



void setupWiFi(){
  delay(10);

  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){delay(500); Serial.print(".");}

  Serial.println("");
  Serial.println("WiFi connected");

  timeClient.begin();
  while (!timeClient.update()) {timeClient.forceUpdate();}

  espClient.setX509Time(timeClient.getEpochTime());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("YasodESP")) {
      Serial.println("connected");
      //client.publish("D2S", "MQTT Connected");
      client.subscribe("S2D");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}

bool initWifi() {                                                                            // Reinitializing WiFi connectivity after awaking
  WiFi.begin(ssid, password);
  delay(500);
  int count=0;
  while((count<50) && (WiFi.status()!=WL_CONNECTED)){digitalWrite(16,HIGH);delay(30);digitalWrite(16, LOW);delay(30);count++;} 
  return WiFi.status()==WL_CONNECTED ? true : false;
 }

bool getWiFi(){
  while (!Serial.available());
  Data = Serial.readString();
  int n = Data.length();
  char str[n+1];
  strcpy(str, Data.c_str());
  char *pch;
  pch = strtok(str, " \n");
  ssid=pch;
  pch = strtok(str, " \n");
  password=pch;
  pch = strtok(str, " \n");
  UserInfo=pch;                       // UserInfo is Char. Storage Capacity of Char is a problem here ???????????????????????????????????????

  
  
  digitalWrite(16, LOW);
  if (initWifi()) {digitalWrite(16, HIGH); Serial.write("Success"); needWiFi=false;}
  else {digitalWrite(16, HIGH); Serial.write("Error");}
  delay(2000);
}



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

void SendSMS(Number,Message)
{
  Serial.println("Sending SMS...");               //Show this message on serial monitor
  sim800l.write("AT+CMGF=1\"\r");                   //Set the module to SMS mode
  delay(100);
  sim800l.write("AT+CMGS=\""+Number+"\"\r");  //Your phone number don't forget to include your country code, example +212123456789"
  delay(500);
  sim800l.write(Message);       //This is the text to send to the phone number, don't make it too long or you have to modify the SoftwareSerial buffer
  delay(500);
  sim800l.write((char)26);// (required according to the datasheet)
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);

}

void setup() {
  pinMode(2, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(Buzzer,OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(16, HIGH);

  sim800l.begin(9600);
  Serial.begin(9600);

  initWifi()
  delay(500);
  if (!SPIFFS.begin()) {Serial.println("Failed to mount file system"); return;}
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  
  File cert = SPIFFS.open("/cert.der", "r");
  if (!cert) Serial.println("Failed to open cert file");
  else Serial.println("Successfully opened cert file");
  delay(1000);
  if (espClient.loadCertificate(cert)) Serial.println("cert loaded");
  else Serial.println("cert load failed");
  
  File private_key = SPIFFS.open("/private.der", "r");
  if (!private_key) Serial.println("Failed to open private_key file");
  else Serial.println("Successfully opened private_key file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key)) Serial.println("private_key loaded");
  else Serial.println("private_key load failed");
  
  File ca = SPIFFS.open("/ca.der", "r");
  if (!ca) Serial.println("Failed to open ca file");
  else Serial.println("Successfully opened ca file");
  delay(1000);
  if (espClient.loadCACert(ca)) Serial.println("ca loaded");
  else Serial.println("ca load failed");
  
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  delay(1000);
  reconnect();
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
      reconnect();
    }
  client.loop();
  delay(100);

  if(New_Message){
    SendSMS(Phone_Num,MSG)
    New_Message=false;
  }

  

}
