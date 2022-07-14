#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

char* ssid = "NOSSID";
char* password = "NOPASSWORD";
char* UserInfo ="";
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
String Type="None";
String MSG="None";
bool New_Message=true;

String Resp_UserApp="";
String Medic="";


int Buzzer=6;
uint8_t Emergency = D3;


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClientSecure espClient;


void callback(char* topic, byte* payload, unsigned int Length) {                     // Take the incomming MQTT Messages and update NodeMCU global variables
  String Data_in = "";
  for (int i = 0; i < Length; i++) {
    Data_in += (char)payload[i];
  }
    Device_ID = getValue(Data_in, ',', 0);
    Type = getValue(Data_in, ',', 1);
    MSG = getValue(Data_in, ',', 2);
  
    char msgr[30]="MQTT receievd";
    Serial.println(msgr);
    
}

PubSubClient client(AWS_endpoint, 8883, callback, espClient);

/*void callback(char* topic, byte* payload, unsigned int Length) {
  Serial.print("Message arrived: [");
  Serial.print(topic);
  Serial.print(": ");
  for (int i=0; i<Length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println("]");
}*/


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



void IntCallback(){
  if (*Status=='Emergency'){
    Status="Normal";
  }

  else{
    char D_ID[32];
    Device_ID_Per.toCharArray(D_ID, 32);
    snprintf(msg, 75, "{\"E-mes\": \"Emergency Situation on Device #%c\"}", D_ID);
    Serial.print("Publish message: "); Serial.println(msg);
    client.publish("D2S", msg);
    Status="Emergency";
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); 
  }
  
}


void setup() {
  pinMode(2, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(Buzzer,OUTPUT);
  digitalWrite(2, HIGH);
  digitalWrite(16, HIGH);
  Serial.begin(115200);

  attachInterrupt(digitalPinToInterrupt(Emergency), IntCallback, RISING);
}

void loop() {
  if (needWiFi){ 
    getWiFi();
    delay(1000);
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
    
    snprintf(msg, 75, "{\"Reg-Mes\": \"%c\"}", UserInfo);                // Sending UserInfo to the server (Registering User)
    Serial.print("Publish message: "); Serial.println(msg);              // UserInfo: "CustomerName/n NIC/n Location/n EmergencyNum1/n EmergencyNum2/n"
    client.publish("D2S", msg);
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
    delay(5000); 
  }

  
  else {
    
    
    digitalWrite(2, LOW);           // Why this GPIO 2 pin ????????????????????????????????????????????????????????????
    if (!client.connected()) {
      reconnect();
    }
    client.loop();

    if (New_Message && (Device_ID_Per==Device_ID)){
      if (Type=="UserRegResp"){
        char Resp_UserApp[64];                           // Have to Display this message on the User Application
        MSG.toCharArray(Resp_UserApp, 64);
        Serial.write(Resp_UserApp);                      // User Application must keep listning to the Serial Channel ?????????????????????????????
      }

      else if (Type=="Medication"){
        Medic=MSG;
        // Pass the message to Text to Audion converter module
      }

      else{
        for (int i=1;i<20;i++){
          digitalWrite(Buzzer,HIGH);
          delay(500);
          digitalWrite(Buzzer,LOW);
          delay(500);   
        }
      }
    }


    // #################################### Local Process or Server Process ##################################################
    long now = millis();
    if (now - lastMsg > 2000){
      lastMsg = now;
      snprintf(msg, 75, "{\"D-Mes\": \"%s\"}", Sens_Read);                     // If local processing is done, no point of sending sensor readings to the server
      Serial.print("Publish message: "); Serial.println(msg);                  // We can trigger the same emergency fuction
      client.publish("D2S", msg);
      Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
      delay(200);
      
      snprintf(msg, 75, "{\"S-Mes\": \"%s\"}", Status);                        // Sending the status of the ESP. Need in both methods.
      Serial.print("Publish message: "); Serial.println(msg);                  
      client.publish("D2S", msg);
      Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
      
    }
    // ######################################################################################
    
    digitalWrite(16, HIGH); delay(1000);
    digitalWrite(16, LOW); delay(1000);
  }
}
