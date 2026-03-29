#include <HCSR04.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "secrets.h"

byte triggerPin = 10;
byte echoPin = 9;
byte LED1 = 4;
byte LED2 = 5;

const char* server = "broker.emqx.io";
const int port = 1883;
const char* clientID = "nano33iot-2.3C";  // must be unique
const char* wave = "ES/WAVE";
const char* pat = "ES/PAT";
unsigned long presentTime;
unsigned long lastTime;


WiFiClient  wifiClient;
PubSubClient client(wifiClient);

void connect()
{
  Serial.print("Connecting to MQTTT");
  while (!client.connected())
  {
    if (client.connect(clientID))
    {
      Serial.println("\nConnected!");
      client.subscribe(wave);
      client.subscribe(pat);
    }
    else 
    {
      Serial.print(".");
      delay(500);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length)
{
  if (strcmp(topic,wave)==0) {leds_on();}
  else if (strcmp(topic,pat)==0) {leds_off();}
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

void leds_on()
{
  digitalWrite(LED1,HIGH);
  digitalWrite(LED2,HIGH);
}
void leds_off()
{
  digitalWrite(LED1,LOW);
  digitalWrite(LED2,LOW);
}

void setup () {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.begin(9600);
  HCSR04.begin(triggerPin, echoPin);
  while (!Serial);

  Serial.print("\nConnecting to WiFi");
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  client.setServer(server, port);
  client.setCallback(callback);
  lastTime =0;
}
void loop () {
  if (!client.connected()) {connect();}
  client.loop();
  presentTime = millis();
  if (presentTime-lastTime>100)
  {
    lastTime = presentTime;
    double* distances = HCSR04.measureDistanceCm();
    if (distances[0]<5)
    {
      client.publish(pat, "Leo Pat trigger");
      Serial.print(distances[0]);
    }
    else if (distances[0]<25 && distances[0]>11)
    {
      client.publish(wave, "Leo wave condition");
      Serial.print(distances[0]);
    }
  }
  
}
