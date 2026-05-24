#include <Arduino_LSM6DSOX.h>
#include <WiFiNINA.h>


char ssid[] = "test";
char pass[] = "password";
WiFiServer server(80);
// -------------------- Pin setup --------------------
const int PIEZO_ANALOG_PIN  = A7;
const int RGB_RED_PIN   = 5;
const int RGB_GREEN_PIN = 6;
const int RGB_BLUE_PIN  = 9;
const int BUTTON_PIN = 3;
// -------------------- Gyroscope thresholds --------------------
// You will need to tune these after testing.
const float SWING_START_GYRO_THRESHOLD = 80.0; 
const float SWING_END_GYRO_THRESHOLD = 15.0;  
const float SWING_TRANSITION_THRESHOLD  = 35.0; 
const unsigned long SWING_END_THRESHOLD = 400;

// -------------------- Interrupt flags --------------------
volatile bool buttonPressed = false;
volatile bool impactDetected = false;
unsigned long buttonWait = 0;
volatile unsigned long impactTimeMicros = 0;
// -------------------- System states --------------------
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
// -------------------- Clubface State --------------------
float faceAngle = 0;
float finalAngle = 0;
unsigned long previousAngleTime = 0;
//enum ClubfaceState {
//  SQUARE,
//  OPEN,
//  CLOSED
//};
// -------------------- Swing data --------------------
String latestSwingType = "No swing yet";
String latestTempo = "N/A";
String latestFaceAngle = "N/A";
String latestPiezo = "N/A";
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

// -------------------- ISRs --------------------
void buttonISR() {
  buttonPressed = true;

}

//void impactISR() {
//  if (state == DOWNSWING)
//  {
//    impactDetected = true;
//    impactTimeMicros = micros();
//  }
//}

// -------------------- LED helper --------------------
void setRGB(bool redOn, bool greenOn, bool blueOn)
  {
    digitalWrite(RGB_RED_PIN,   redOn   ? HIGH : LOW);
    digitalWrite(RGB_GREEN_PIN, greenOn ? HIGH : LOW);
    digitalWrite(RGB_BLUE_PIN,  blueOn  ? HIGH : LOW);
  }


// -------------------- Sensor helpers --------------------
float getGyroMagnitude(float gx, float gy, float gz) {
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
void handleWebClient() {
  WiFiClient client = server.available();

  if (!client) {
    return;
  }
  unsigned long startTime = millis();
  // Wait for browser request
  while (client.connected()) 
  {
    if (client.available()) 
    {
      String request = client.readStringUntil('\r');
      client.flush();

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      client.println("<!DOCTYPE html>");
      client.println("<html>");
      client.println("<head>");
      client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
      client.println("<meta http-equiv='refresh' content='2'>");

      client.println("<style>");
      client.println("body{font-family:Arial,Helvetica,sans-serif;background:#f4f7fb;color:#111;margin:0;padding:18px;}");
      client.println("h1{font-size:28px;margin:0 0 6px 0;color:#111;}");
      client.println(".subtitle{font-size:14px;color:#555;margin-bottom:18px;}");
      client.println(".status{background:#ffffff;border-left:6px solid #2e7d32;border-radius:12px;padding:14px;margin-bottom:18px;box-shadow:0 2px 8px rgba(0,0,0,0.12);}");
      client.println(".statusLabel{font-size:13px;color:#555;text-transform:uppercase;letter-spacing:0.05em;}");
      client.println(".statusValue{font-size:26px;font-weight:bold;margin-top:4px;color:#2e7d32;}");
      client.println(".grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;}");
      client.println(".card{background:#ffffff;border-radius:14px;padding:16px;min-height:95px;box-shadow:0 2px 8px rgba(0,0,0,0.12);border:1px solid #dde3ea;}");
      client.println(".label{font-size:13px;color:#555;margin-bottom:10px;text-transform:uppercase;letter-spacing:0.04em;}");
      client.println(".value{font-size:24px;font-weight:bold;color:#111;line-height:1.1;}");
      client.println(".unit{font-size:15px;color:#666;font-weight:normal;}");
      client.println(".footer{font-size:12px;color:#666;margin-top:18px;text-align:center;}");
      client.println("@media(max-width:520px){.grid{grid-template-columns:1fr;}h1{font-size:24px}.value{font-size:22px;}}");
      client.println("</style>");

      client.println("</head>");
      client.println("<body>");

      client.println("<h1>Smart Golf Swing Analyser</h1>");
      client.println("<div class='subtitle'>Latest swing result</div>");

      client.println("<div class='status'>");
      client.println("<div class='statusLabel'>Swing Type</div>");
      client.println("<div class='statusValue'>" + latestSwingType + "</div>");
      client.println("</div>");

      client.println("<div class='grid'>");

      client.println("<div class='card'>");
      client.println("<div class='label'>Swing Speed</div>");
      client.println("<div class='value'>" + String(latestSwingSpeedKmh, 1) + " <span class='unit'>km/h</span></div>");
      client.println("</div>");

      client.println("<div class='card'>");
      client.println("<div class='label'>Backswing Time</div>");
      client.println("<div class='value'>" + String(latestBackswingTime) + " <span class='unit'>ms</span></div>");
      client.println("</div>");

      client.println("<div class='card'>");
      client.println("<div class='label'>Total Swing Time</div>");
      client.println("<div class='value'>" + String(latestTotalSwingTime) + " <span class='unit'>ms</span></div>");
      client.println("</div>");

      client.println("<div class='card'>");
      client.println("<div class='label'>Tempo</div>");
      client.println("<div class='value'>" + latestTempo + "</div>");
      client.println("</div>");

      client.println("<div class='card'>");
      client.println("<div class='label'>Face Rotation</div>");
      client.println("<div class='value'>" + latestFaceAngle + "</div>");
      client.println("</div>");

      client.println("<div class='card'>");
      client.println("<div class='label'>Piezo / Strike</div>");
      client.println("<div class='value'>" + latestPiezo + "</div>");
      client.println("</div>");

      client.println("</div>");

      client.println("<div class='footer'>Page refreshes every 2 seconds</div>");

      client.println("</body>");
      client.println("</html>");

      break;
    }
    else 
    {
      if (millis()-startTime>1000) {client.stop(); return;}
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

  //pinMode(PIEZO_DIGITAL_PIN, INPUT);
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

  // Depending on your piezo module, this may need to be RISING or FALLING.
  // If impact never triggers, try changing RISING to FALLING.
  //attachInterrupt(digitalPinToInterrupt(PIEZO_DIGITAL_PIN), impactISR, RISING);

  state = STANDBY;
  setRGB(true, true, false);

  Serial.println("System ready. Press button to arm.");
}

// -------------------- Main loop --------------------
void loop() {
  if (state == STANDBY || state == PROCESSING) {handleWebClient();}
  if (state == STANDBY) {
    setRGB(true, true, false);

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
        peakGyro = gyroMag;
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
      swingEndTime = millis();
      state = PROCESSING; 
    }   
  }

  else if (state == PROCESSING) {
    setRGB(false,false,false);

    unsigned long totalSwingTime = swingEndTime - swingStartTime;
    buttonWait = millis()+4000;

    if (realStrike)
    {
      float tempoRatio = (float)backswingTime / (float)downswingTime;
      latestTempo = String(tempoRatio,1)+":1";
      latestSwingType = "Real strike";
      latestFaceAngle = String(finalAngle,1)+" degrees";
      latestPiezo = String(piezoAnalogValue);
    } 
    else 
    {
      latestSwingType = "Practice swing";
      latestTempo = "N/A";
      latestFaceAngle = "N/A";
      latestPiezo = "No impact detected";
    }


    
    float kmh = (peakGyro*3.14/180)*1.33*3.6;
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
