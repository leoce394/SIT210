#include <Arduino_LSM6DSOX.h>
#include <WiFiNINA.h>


char ssid[] = "test";
char pass[] = "password";
WiFiServer server(80);
// -------------------- Pin setup --------------------
const int PIEZO_DIGITAL_PIN = 2;
const int PIEZO_ANALOG_PIN  = A7;

const int RGB_RED_PIN   = 5;
const int RGB_GREEN_PIN = 6;
const int RGB_BLUE_PIN  = 9;

const int BUTTON_PIN = 3;


// -------------------- Swing thresholds --------------------
// You will need to tune these after testing.
const float SWING_START_GYRO_THRESHOLD = 80.0;   // deg/s
const float SWING_END_GYRO_THRESHOLD = 15.0;  
const float SWING_TRANSITION_THRESHOLD  = 35.0;    // deg/s

const unsigned long SWING_END_THRESHOLD = 400; // ms below threshold before swing ends


// -------------------- Interrupt flags --------------------
volatile bool buttonPressed = false;
volatile bool impactDetected = false;
unsigned long buttonWait = 0;

volatile unsigned long impactTimeMicros = 0;


// -------------------- State machine --------------------
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

// -------------------- Interrupt service routines --------------------
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

void setLedIdle() {
  // Orange = red + green
  setRGB(true, true, false);
}

void setLedArmed() {
  setRGB(false, true, false);
}

void setLedRecording() {
  setRGB(false, false, true);
}

void setLedProcessing() {
  // Purple = red + blue
  setRGB(true, false, true);
}

void setLedError() {
  setRGB(true, false, false);
}

void setLedSuccess() {
  // White = red + green + blue
  setRGB(true, true, true);
}
void setLedDownswing() {
  // White = red + green + blue
  setRGB(true, true, false);
}

// -------------------- Sensor helpers --------------------
float getGyroMagnitude(float gx, float gy, float gz) {
  return sqrt(gx * gx + gy * gy + gz * gz);
}

float getAccelMagnitude(float ax, float ay, float az) {
  return sqrt(ax * ax + ay * ay + az * az);
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
  downswingTime = millis() - swingStartTime - backswingTime;
  finalAngle = faceAngle;

  state = PROCESSING;

  Serial.println("Impact detected.");
}
void handleWebClient() {
  WiFiClient client = server.available();

  if (!client) {
    return;
  }

  // Wait for browser request
  while (client.connected()) {
    if (client.available()) {
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
      client.println("body{font-family:Arial;background:#111;color:#eee;padding:20px;}");
      client.println(".card{background:#222;border-radius:12px;padding:18px;margin:12px 0;}");
      client.println(".value{font-size:28px;font-weight:bold;color:#7CFC00;}");
      client.println(".label{color:#aaa;font-size:14px;}");
      client.println("</style>");
      client.println("</head>");

      client.println("<body>");
      client.println("<h1>Smart Golf Swing Analyser</h1>");

      client.println("<div class='card'><div class='label'>Swing Type</div><div class='value'>" + latestSwingType + "</div></div>");

      client.println("<div class='card'><div class='label'>Estimated 7 Iron Swing Speed</div><div class='value'>" + String(latestSwingSpeedKmh, 1) + " km/h</div></div>");

      client.println("<div class='card'><div class='label'>Backswing Time</div><div class='value'>" + String(latestBackswingTime) + " ms</div></div>");

      client.println("<div class='card'><div class='label'>Total Swing Time</div><div class='value'>" + String(latestTotalSwingTime) + " ms</div></div>");

      client.println("<div class='card'><div class='label'>Tempo</div><div class='value'>" + latestTempo + "</div></div>");

      client.println("<div class='card'><div class='label'>Face Angle / Rotation</div><div class='value'>" + latestFaceAngle + "</div></div>");

      client.println("<div class='card'><div class='label'>Piezo Value</div><div class='value'>" + latestPiezo + "</div></div>");

      client.println("<p>Page refreshes every 2 seconds.</p>");

      client.println("</body>");
      client.println("</html>");

      break;
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

  pinMode(PIEZO_DIGITAL_PIN, INPUT);
  pinMode(PIEZO_ANALOG_PIN, INPUT);

  setLedError();

  Serial.println("Starting smart golf swing analyser prototype...");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    state = ERROR_STATE;
    while (1) {
      setLedError();
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
    
    setLedError();
    
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
  setLedIdle();

  Serial.println("System ready. Press button to arm.");
}

// -------------------- Main loop --------------------
void loop() {
  if (state == STANDBY || state == PROCESSING) {handleWebClient();}
  if (state == STANDBY) {
    setLedIdle();

    if (buttonPressed && millis()>buttonWait) 
    {
      buttonPressed = false;
      resetSwingData();
      previousAngleTime = micros();
      state = ARMED;
      setLedArmed();

      Serial.println();
      Serial.println("System armed. Waiting for swing...");
    }
  }

  else if (state == ARMED) {
    float gx, gy, gz;

    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);

      float gyroMag = getGyroMagnitude(gx, gy, gz);

      if (gyroMag > SWING_START_GYRO_THRESHOLD) {
        swingStartTime = millis();
        peakGyro = gyroMag;

        state = BACKSWING;
        setLedRecording();

        Serial.println("Swing started.");
      }
    }
  }

  else if (state == BACKSWING) {
    float ax, ay, az;
    float gx, gy, gz;

    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);
      faceAngleUpdate(gx);
      float gyroMag = getGyroMagnitude(gx, gy, gz);

      if (gyroMag < SWING_TRANSITION_THRESHOLD) {
        backswingTime = millis() - swingStartTime;
        state = DOWNSWING;
        setLedDownswing();
        lastMotionTime = millis();
      }
    }
  }

  else if (state == DOWNSWING) {
    float ax, ay, az;
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

    

    else if (millis()-lastMotionTime>SWING_END_THRESHOLD) 
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
    Serial.println();
    Serial.println("----- Swing Result -----");

    if (realStrike)
    {
      float tempoRatio = (float)backswingTime / (float)downswingTime;
      latestTempo = String(tempoRatio,1)+":1";
      latestSwingType = "Real strike";
      latestFaceAngle = String(finalAngle,1)+" degrees";
      latestPiezo = String(piezoAnalogValue);
      Serial.println("Swing type: Real strike");
      Serial.print("Swing Tempo: Backswing/Downswing");
      Serial.print(tempoRatio, 1);
      Serial.println(":1");
      Serial.print("change in clubface (+closed), (-open)");
      Serial.print(finalAngle);
      Serial.println(" degrees");
      Serial.print("Piezo analog value: ");
      Serial.println(piezoAnalogValue);
    } 
    else 
    {
      latestSwingType = "Practice swing";
      latestTempo = "N/A";
      latestFaceAngle = "N/A";
      latestPiezo = "No impact detected";
      Serial.println("Swing type: Practice swing");
      Serial.print("Total swing time: ");
      Serial.print(totalSwingTime);
      Serial.println(" ms");
      Serial.print("Backswing Time: ");
      Serial.print(backswingTime);
      Serial.println(" ms");
    }


    Serial.print("Estimated 7 iron Swing Speed ");
    float kmh = (peakGyro*3.14/180)*0.62*3.6;
    latestSwingSpeedKmh = kmh;
    latestBackswingTime = backswingTime;
    latestTotalSwingTime = totalSwingTime;
    Serial.print(kmh);
    Serial.println(" km/h");


    Serial.println("------------------------");
    Serial.println();

    setLedSuccess();
    delay(500);

    state = STANDBY;
    setLedIdle();

    Serial.println("System ready. Press button to arm.");
  }

  else if (state == ERROR_STATE) {
    setLedError();
  }
}
