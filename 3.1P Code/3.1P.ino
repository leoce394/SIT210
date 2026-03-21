#include <WiFiNINA.h>
#include "secrets.h"
#include <BH1750.h>
#include <Wire.h>


WiFiClient  client;
BH1750 lightMeter;
unsigned long presentTime;
unsigned long startTime;

char   HOST_NAME[] = "maker.ifttt.com";
String PATH_NAME_SUN   = "/trigger/sensor_reading/with/key/bUUqL0IsIv8k32jdT6iBAN"; // change your EVENT-NAME and YOUR-KEY
String PATH_NAME_DARK = "/trigger/sensor_reading_dark/with/key/bUUqL0IsIv8k32jdT6iBAN";
bool sunny;


void setup() {
  Serial.begin(9600);
  Wire.begin();
  lightMeter.begin();
  while (!Serial);

  Serial.print("Connecting to WiFi");
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  startTime = millis();
  if (round(lightMeter.readLightLevel())>400){sunny = true;}
  else {sunny = false;}
}

int readLight()
{
  int light = round(lightMeter.readLightLevel());
  Serial.println(light);
  return light;
}
void sendEmail(String PATH_NAME, String reading)
{
  if (client.connect(HOST_NAME, 80))
   {
        client.println("GET " + PATH_NAME + "?value1="+reading+  " HTTP/1.1");
        client.println("Host: " + String(HOST_NAME));
        client.println("Connection: close");
        client.println();
    }
  while (client.connected() || client.available()) 
  {
    while (client.available()) 
    {
      char c = client.read();
      Serial.print(c);
    }
  }
  client.stop();
}



void loop() {
    presentTime = millis();

    if (presentTime - startTime >= 5000)
    {
      int reading = readLight();
      if (!sunny && reading>=400) {sendEmail(PATH_NAME_SUN, String(reading)); sunny = true;}
      else if (sunny && reading<400) {sendEmail(PATH_NAME_DARK, String(reading)); sunny = false;}
      startTime = presentTime;
    }
}

    