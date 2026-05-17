#include <BH1750.h>
#include <Wire.h>
#include <ArduinoBLE.h>

BH1750 lightMeter;
BLEService lightService("19B10000-EB67-592D-B9AC-A76E4D89BC63");
BLEStringCharacteristic command("19B10001-EB67-592D-B9AC-A76E4D89BC63", BLEWrite,20);

const int bathroomPin = 3;
const int hallwayPin = 4;
const int exhaustPin = 2;
//19B10001-EB67-592D-B9AC-A76E4D89BC63
void setup() {
  
  Serial.begin(9600);
  while (!Serial);
  Wire.begin();
  lightMeter.begin();
  Serial.println("hey");

  pinMode(bathroomPin, OUTPUT);
  pinMode(hallwayPin, OUTPUT);
  pinMode(exhaustPin, OUTPUT);
  digitalWrite(exhaustPin, HIGH);

  if (!BLE.begin()) {
    Serial.println("BLE failed");
    while (1);
  }

  BLE.setLocalName("Nano");
  BLE.setAdvertisedService(lightService);

  lightService.addCharacteristic(command);
  BLE.addService(lightService);

  BLE.advertise();

  Serial.println("NanoLight BLE ready");
}

void loop() {
  BLEDevice central = BLE.central();
  if (central)
  {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected())
    {
      if (command.written())
      {
        String message = command.value();
        Serial.println(message);
        bool dark;
        if (lightMeter.readLightLevel()<100) {dark =true;}
        else{dark=false;}
      
        if (message == "bathroom on" &&dark) {digitalWrite(bathroomPin,HIGH);}
        else if (message == "hallway on" &&dark) {digitalWrite(hallwayPin,HIGH);}
        else if (message == "all on" &&dark)
        {
          digitalWrite(bathroomPin,HIGH);
          digitalWrite(hallwayPin,HIGH);
          digitalWrite(exhaustPin,LOW);
        }
        
        
        if (message == "fan on") {digitalWrite(exhaustPin,LOW);}
        else if (message == "bathroom off") {digitalWrite(bathroomPin,LOW);}
        else if (message == "hallway off") {digitalWrite(hallwayPin,LOW);}
        else if (message == "fan off") {digitalWrite(exhaustPin,HIGH);}
        else if (message == "all off")
        {
          digitalWrite(bathroomPin,LOW);
          digitalWrite(hallwayPin,LOW);
          digitalWrite(exhaustPin,HIGH);
        }
      }
    }
    Serial.print("Disconnected");
  }

}
