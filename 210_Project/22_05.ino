#include <Arduino_LSM6DSOX.h>
#include <WiFiNINA.h>


char ssid[] = "test";
char pass[] = "password";
WiFiServer server(80);
// Pins
const int PIEZO_ANALOG_PIN  = A7;
const int RGB_RED_PIN   = 5;
const int RGB_GREEN_PIN = 6;
const int RGB_BLUE_PIN  = 9;
const int BUTTON_PIN = 3;
// Gyro magnitude thresholds
const float SWING_START_GYRO_THRESHOLD = 80.0; 
const float SWING_END_GYRO_THRESHOLD = 15.0;  
const float SWING_TRANSITION_THRESHOLD  = 35.0; 
const unsigned long SWING_END_THRESHOLD = 400;

// Interrupt fields
volatile bool buttonPressed = false;
volatile bool impactDetected = false;
unsigned long buttonWait = 0;
volatile unsigned long impactTimeMicros = 0;
// State machine
enum SystemState {
  SETUP_STATE,
  WIFI_SETUP,
  STANDBY,
  ARMED,
  BACKSWING,
  DOWNSWING,
  PROCESSING,
  ERROR_STATE
};
volatile SystemState state = SETUP_STATE;
// Clubface angle metrics
float faceAngle = 0;
float finalAngle = 0;
unsigned long previousAngleTime = 0;
// Swing data fields
String latestSwingType = "No swing yet";
String latestTempo = "N/A";
String latestFaceAngle = "N/A";
String latestHarshness = "N/A";
unsigned long latestTotalSwingTime = 0;
unsigned long latestBackswingTime = 0;
float latestSwingSpeedKmh = 0.0;
unsigned long swingStartTime = 0;
unsigned long backswingTime = 0;
unsigned long downswingTime = 0;
unsigned long lastMotionTime = 0;
unsigned long swingEndTime = 0;


bool realStrike = false;

float peakGyro = 0.0;
float peakAccel = 0.0;

const int pizeoThreshold = 200;
int piezoAnalogValue = 0;

//ISR
void buttonISR() 
{
  buttonPressed = true;
}
// RGB LED setter
void setRGB(bool redOn, bool greenOn, bool blueOn)
  {
    digitalWrite(RGB_RED_PIN,   redOn   ? HIGH : LOW);
    digitalWrite(RGB_GREEN_PIN, greenOn ? HIGH : LOW);
    digitalWrite(RGB_BLUE_PIN,  blueOn  ? HIGH : LOW);
  }


// Calculation and update methods
float getGyroMagnitude(float gx, float gy, float gz) 
{
  return sqrt(gx * gx + gy * gy + gz * gz);
}

void faceAngleUpdate(float gx)
{
  unsigned long currentTime = micros();
  faceAngle += gx * ((currentTime - previousAngleTime)/1000000.0);
  previousAngleTime = currentTime;
}
void impactUpdate()
{
  piezoAnalogValue = analogRead(PIEZO_ANALOG_PIN);
  if (piezoAnalogValue>pizeoThreshold) {impactDetected=true;}
  else {piezoAnalogValue=0;}
}

void resetSwingData() {
  swingStartTime = 0;
  swingEndTime = 0;
  lastMotionTime = 0;
  backswingTime = 0;
  downswingTime = 0;
  
  faceAngle = 0;
  finalAngle = 0;
  previousAngleTime = 0;

  realStrike = false;

  peakGyro = 0.0;
  peakAccel = 0.0;

  piezoAnalogValue = 0;

  impactDetected = false;
}
void handleStrike()
{
  impactDetected = false;
  realStrike = true;
  swingEndTime = millis();
  downswingTime = swingEndTime - swingStartTime - backswingTime;
  finalAngle = faceAngle;
  state = PROCESSING;
  Serial.println("Impact detected.");
}
void handleWebpage() {
  WiFiClient client = server.available();

  if (!client) {
    return;
  }

  unsigned long startTime = millis();

  while (client.connected()) {
    if (client.available()) {
      String request = client.readStringUntil('\r');
      client.flush();

      client.println(F("HTTP/1.1 200 OK"));
      client.println(F("Content-Type: text/html"));
      client.println(F("Connection: close"));
      client.println(F("Cache-Control: no-store"));
      client.println();

      client.println(F("<!DOCTYPE html>"));
      client.println(F("<html>"));
      client.println(F("<head>"));
      client.println(F("<meta name='viewport' content='width=device-width, initial-scale=1'>"));
      client.println(F("<meta http-equiv='refresh' content='2'>"));

      client.println(F("<style>"));
      client.println(F("body{font-family:Arial;background:#f6f8fa;color:#111;margin:0;padding:16px;}"));
      client.println(F("h1{font-size:26px;margin:0 0 4px 0;}"));
      client.println(F(".sub{font-size:14px;color:#555;margin-bottom:14px;}"));
      client.println(F(".status{background:#fff;border-left:6px solid #2e7d32;border-radius:10px;padding:14px;margin-bottom:12px;border:1px solid #ddd;}"));
      client.println(F(".grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;}"));
      client.println(F(".card{background:#fff;border-radius:10px;padding:14px;border:1px solid #ddd;}"));
      client.println(F(".label{font-size:13px;color:#555;margin-bottom:8px;}"));
      client.println(F(".value{font-size:22px;font-weight:bold;color:#111;}"));
      client.println(F(".footer{font-size:12px;color:#666;text-align:center;margin-top:14px;}"));
      client.println(F("@media(max-width:520px){.grid{grid-template-columns:1fr;}.value{font-size:21px;}}"));
      client.println(F("</style>"));

      client.println(F("</head>"));
      client.println(F("<body>"));

      client.println(F("<h1>Smart Golf Swing Analyser</h1>"));
      client.println(F("<div class='sub'>Latest swing result</div>"));

      client.println(F("<div class='status'>"));
      client.println(F("<div class='label'>Swing Type</div>"));
      client.print(F("<div class='value'>"));
      client.print(latestSwingType);
      client.println(F("</div>"));
      client.println(F("</div>"));

      client.println(F("<div class='grid'>"));

      client.println(F("<div class='card'><div class='label'>Swing Speed</div><div class='value'>"));
      client.print(String(latestSwingSpeedKmh, 1));
      client.println(F(" km/h</div></div>"));

      client.println(F("<div class='card'><div class='label'>Backswing Time</div><div class='value'>"));
      client.print(String(latestBackswingTime));
      client.println(F(" ms</div></div>"));

      client.println(F("<div class='card'><div class='label'>Total Swing Time</div><div class='value'>"));
      client.print(String(latestTotalSwingTime));
      client.println(F(" ms</div></div>"));

      client.println(F("<div class='card'><div class='label'>Tempo</div><div class='value'>"));
      client.print(latestTempo);
      client.println(F("</div></div>"));

      client.println(F("<div class='card'><div class='label'>Face Rotation</div><div class='value'>"));
      client.print(latestFaceAngle);
      client.println(F("</div></div>"));

      client.println(F("<div class='card'><div class='label'>Harshness Factor</div><div class='value'>"));
      client.print(latestHarshness);
      client.println(F("</div></div>"));

      client.println(F("</div>"));

      client.println(F("<div class='footer'>Page refreshes every 2 seconds</div>"));

      client.println(F("</body>"));
      client.println(F("</html>"));

      break;
    }

    if (millis() - startTime > 1000) {
      client.stop();
      return;
    }
  }

  delay(1);
  client.stop();
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(9600);
  delay(2000);

  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIEZO_ANALOG_PIN, INPUT);

  setRGB(true, false, false);

  Serial.println("Starting smart golf swing analyser prototype...");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    state = ERROR_STATE;
    while (1) {
      setRGB(true, false, false);
    }
  }
  Serial.println("IMU initialized successfully.");
  Serial.print("Connecting to WiFi... ");
  Serial.println(ssid);
  int status = WL_IDLE_STATUS;
  while (status!=WL_CONNECTED)
  {
    status = WiFi.begin(ssid,pass);
    delay(2000);
    state = ERROR_STATE;
    
    setRGB(true, false, false);
    
    Serial.print(".");
  }
  Serial.println("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
  server.begin();

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");

  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  
  state = STANDBY;
  setRGB(true, true, false);

  Serial.println("System ready. Press button to arm.");
}

// Main logic via State Machine
void loop() {
  if (state == STANDBY) 
  {
    setRGB(true, true, false);
    handleWebpage();
    if (buttonPressed && millis()>buttonWait) 
    {
      buttonPressed = false;
      resetSwingData();
      state = ARMED;
      setRGB(false, true, false);

      Serial.println();
      Serial.println("System armed. Waiting for swing...");
    }
    else {buttonPressed = false;}
  }

  else if (state == ARMED) {
    float gx, gy, gz;

    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);

      float gyroMag = getGyroMagnitude(gx, gy, gz);

      if (gyroMag > SWING_START_GYRO_THRESHOLD) {
        swingStartTime = millis();
        previousAngleTime = micros();

        state = BACKSWING;
        setRGB(false, false, true);

        Serial.println("Swing started.");
      }
    }
  }

  else if (state == BACKSWING) {
    float gx, gy, gz;

    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);
      faceAngleUpdate(gx);
      float gyroMag = getGyroMagnitude(gx, gy, gz);

      if (gyroMag < SWING_TRANSITION_THRESHOLD) {
        backswingTime = millis() - swingStartTime;
        state = DOWNSWING;
        setRGB(true, true, false);
        lastMotionTime = millis();
      }
    }
  }

  else if (state == DOWNSWING) {
    float gx, gy, gz;
    impactUpdate();
    if (impactDetected)
    {
      handleStrike();
      return;
    }
    if (IMU.gyroscopeAvailable()) 
    {
      IMU.readGyroscope(gx, gy, gz);
      impactUpdate();
      if (impactDetected) {handleStrike(); return;}
      else
      {
        faceAngleUpdate(gx);
        float gyroMag = getGyroMagnitude(gx, gy, gz);
  
        if (gyroMag > peakGyro) {
          peakGyro = gyroMag;
        }
        if (gyroMag>SWING_END_GYRO_THRESHOLD) {lastMotionTime = millis();}
      }
    }
    if (millis()-lastMotionTime>SWING_END_THRESHOLD) 
    {
      realStrike = false;
      swingEndTime = millis()-SWING_END_THRESHOLD;
      state = PROCESSING; 
    }   
  }

  else if (state == PROCESSING) {
    setRGB(false,false,false);

    unsigned long totalSwingTime = swingEndTime - swingStartTime;
    buttonWait = millis()+4000;
    float kmh = (peakGyro*3.14/180)*1.33*3.6;

    if (realStrike)
    {
      float tempoRatio = (float)backswingTime / (float)downswingTime;
      latestTempo = String(tempoRatio,1)+":1";
      latestSwingType = "Real strike";
      if (finalAngle>0){latestFaceAngle = String(finalAngle,1)+" degrees CLOSED";}
      else {latestFaceAngle = String(abs(finalAngle),1)+" degrees OPEN";}
      latestHarshness = String(map(piezoAnalogValue,0,1023,0,100)/kmh);
    } 
    else 
    {
      latestSwingType = "Practice swing";
      latestTempo = "N/A";
      latestFaceAngle = "N/A";
      latestHarshness = "No impact detected";
    }
    latestSwingSpeedKmh = kmh;
    latestBackswingTime = backswingTime;
    latestTotalSwingTime = totalSwingTime;

    setRGB(true, true, true);
    delay(500);

    state = STANDBY;
    setRGB(true, true, false);
  }

  else if (state == ERROR_STATE) {
    setRGB(true, false, false);
  }
}
