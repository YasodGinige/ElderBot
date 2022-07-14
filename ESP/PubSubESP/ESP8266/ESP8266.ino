#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid = "BELL4G-CB08";
const char* password = "31900savitha";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* AWS_endpoint = "a3diuj9r5yc78a-ats.iot.us-east-2.amazonaws.com";

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived: [");
  Serial.print(topic);
  Serial.print(": ");
  for (int i=0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println("]");
}

WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient);
long lastMsg = 0;
char msg[50];
int value=0;

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
    if (client.connect("PamuESP")) {
      Serial.println("connected");
      client.publish("outTopic", "hello world");
      client.subscribe("inTopic");
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

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  pinMode(LED_BUILTIN, OUTPUT);
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
  setupWiFi();
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000){
    lastMsg = now;
    ++value;
    snprintf(msg, 75, "{\"device\": \"PamuESP\",\"message\": \"hello world #%1d\"}", value);
    Serial.print("Publish message: "); Serial.println(msg);
    client.publish("outTopic", msg);
    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  }

  digitalWrite(LED_BUILTIN, HIGH); delay(1000);
  digitalWrite(LED_BUILTIN, LOW); delay(1000);

}
