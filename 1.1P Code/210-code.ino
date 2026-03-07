#define PORCH_LIGHT 3
#define HOUSE_LIGHT 2
#define SWITCH_PIN 4
void setup() {
  pinMode(PORCH_LIGHT, OUTPUT);
  pinMode(HOUSE_LIGHT, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);

}
bool read_button()
{
  if (digitalRead(SWITCH_PIN) == HIGH) {return true;}
  else {return false;}
}
void porch_light_switch()
{
  if (digitalRead(PORCH_LIGHT) == HIGH) {digitalWrite(PORCH_LIGHT, LOW);}
  else {digitalWrite(PORCH_LIGHT, HIGH);}
}
void house_light_switch()
{
  if (digitalRead(HOUSE_LIGHT) == HIGH) {digitalWrite(HOUSE_LIGHT, LOW);}
  else {digitalWrite(HOUSE_LIGHT, HIGH);}
}
void loop() {
  if (read_button())
  {
    porch_light_switch();
    house_light_switch();
    delay(30000);
    porch_light_switch();
    delay(30000);
    house_light_switch();
  }

}
