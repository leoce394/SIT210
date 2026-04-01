#include <BH1750.h>
#include <Wire.h>

#define PORCH_LIGHT 4
#define HOUSE_LIGHT 5
#define SWITCH_PIN 2
#define MOTION_PIN 3

unsigned long presentTime;
unsigned long startTime;
bool lightsOn;
int stage = 0;
BH1750 lightMeter;

void setup() {
  pinMode(PORCH_LIGHT, OUTPUT);
  pinMode(HOUSE_LIGHT, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);
  pinMode(MOTION_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN),switchISR,RISING);
  attachInterrupt(digitalPinToInterrupt(MOTION_PIN),motionISR,RISING);
  lightsOn = false;
  Serial.begin(9600);
  while (!Serial);
  Serial.println("starting...");
  Wire.begin();
  lightMeter.begin();
}

void motionISR()
{
  Serial.println("motion detected");
  lightsOn = true;
}
void switchISR()
{
  Serial.println("switch flip detected");
  lightsOn = true; 
}
void porch_light_switch(String flip)
{
  if (flip == "ON") {digitalWrite(PORCH_LIGHT, HIGH);}
  else if (flip == "OFF") {digitalWrite(PORCH_LIGHT, LOW);}
}
void house_light_switch(String flip)
{
  if (flip == "ON") {digitalWrite(HOUSE_LIGHT, HIGH);}
  else if (flip == "OFF") {digitalWrite(HOUSE_LIGHT, LOW);}
}

void loop() {
  
  presentTime = millis();
  if (lightsOn && lightMeter.readLightLevel() < 100)
  {
    lightsOn = false;
    porch_light_switch("ON");
    house_light_switch("ON");
    startTime = presentTime;
    stage = 1;
  }
  else if (stage == 1 && presentTime - startTime >=30000)
  {
    porch_light_switch("OFF");
    startTime = presentTime;
    stage = 2;
  }
  else if (stage == 2 && presentTime - startTime >=30000)
  {
    house_light_switch("OFF");
    stage = 0;
  }
  
}
