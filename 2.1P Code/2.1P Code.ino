#include <WiFiNINA.h>
#include "secrets.h"
#include "ThingSpeak.h"
#include <DHT11.h>
#include <BH1750.h>
#include <Wire.h>

WiFiClient  client;
unsigned long presentTime;
unsigned long startTime;

DHT11 dht11(2);
BH1750 lightMeter;
void setup() {
  Serial.begin(9600);
  Wire.begin();
  lightMeter.begin();

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected!");

  ThingSpeak.begin(client);

  startTime = millis();
}
int readTemperature()
{
  int temp = dht11.readTemperature();
  Serial.println(temp);
  return temp;
}
int readLight()
{
  int light = round(lightMeter.readLightLevel());
  Serial.println(light);
  return round(light);
}
void setValues()
{
  ThingSpeak.setField(1,readTemperature());
  ThingSpeak.setField(2,readLight());
}

void loop() {
  presentTime = millis();
  if (presentTime - startTime >= 30000)
  {
    setValues();
    startTime = presentTime;
    int x = ThingSpeak.writeFields(SECRET_CH_ID, SECRET_WRITE_APIKEY);
    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  }

}
