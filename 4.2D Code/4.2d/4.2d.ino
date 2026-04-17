#include <WiFiNINA.h>
#include "secrets.h"

WiFiServer server(80);
byte LIVING_ROOM = 4;
byte BATHROOM = 5;
byte CLOSET = 6;

void setup() {
  Serial.begin(9600);
  pinMode(LIVING_ROOM, OUTPUT);
  pinMode(BATHROOM, OUTPUT);
  pinMode(CLOSET, OUTPUT);
  while (!Serial);

  Serial.print("\nConnecting to WiFi");
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}
void toggleLivingRoom()
{
  if (digitalRead(LIVING_ROOM) == HIGH) {digitalWrite(LIVING_ROOM,LOW);}
  else {digitalWrite(LIVING_ROOM,HIGH);}
}
void toggleBathroom()
{
  if (digitalRead(BATHROOM) == HIGH) {digitalWrite(BATHROOM,LOW);}
  else {digitalWrite(BATHROOM,HIGH);}
}
void toggleCloset()
{
  if (digitalRead(CLOSET) == HIGH) {digitalWrite(CLOSET,LOW);}
  else {digitalWrite(CLOSET,HIGH);}
}

void loop() {
  WiFiClient client = server.available();
  if (client)
  {
    Serial.print("new client");
    String s = client.readStringUntil('\n');
    server.println(s);
    if (s.indexOf("livingRoom")>=0) {toggleLivingRoom();}
    else if (s.indexOf("bathroom")>=0) {toggleBathroom();}
    else if (s.indexOf("closet")>=0) {toggleCloset();}
  }

}
