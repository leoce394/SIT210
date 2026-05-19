#include <Arduino_LSM6DSOX.h>

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
const float SWING_TRANSITION_THRESHOLD  = 35.0;    // deg/s

const unsigned long SWING_END_THRESHOLD = 400; // ms below threshold before swing ends


// -------------------- Interrupt flags --------------------
volatile bool buttonPressed = false;
volatile bool impactDetected = false;

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


SystemState state = SETUP_STATE;
// -------------------- Clubface State --------------------
float faceAngle = 0;
float finalAngle = 0;
unsigned long previousAngleTime = 0;
enum ClubfaceState {
  SQUARE,
  OPEN,
  CLOSED
};
// -------------------- Swing data --------------------
unsigned long swingStartTime = 0;
unsigned long backswingTime = 0;
unsigned long downswingTime = 0;
unsigned long lastMotionTime = 0;
unsigned long swingEndTime = 0;


bool realStrike = false;

float peakGyro = 0.0;
float peakAccel = 0.0;

int piezoAnalogValue = 0;

// -------------------- Interrupt service routines --------------------
void buttonISR() {
  buttonPressed = true;

}

void impactISR() {
  impactDetected = true;
  impactTimeMicros = micros();
}

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

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");

  Serial.print("Gyroscope sample rate = ");
  Serial.print(IMU.gyroscopeSampleRate());
  Serial.println(" Hz");

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // Depending on your piezo module, this may need to be RISING or FALLING.
  // If impact never triggers, try changing RISING to FALLING.
  attachInterrupt(digitalPinToInterrupt(PIEZO_DIGITAL_PIN), impactISR, RISING);

  state = STANDBY;
  setLedIdle();

  Serial.println("System ready. Press button to arm.");
}

// -------------------- Main loop --------------------
void loop() {
  if (state == STANDBY) {
    setLedIdle();

    if (buttonPressed) {
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

    if (IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gx, gy, gz);
      faceAngleUpdate(gx);
      float gyroMag = getGyroMagnitude(gx, gy, gz);

      if (gyroMag > peakGyro) {
        peakGyro = gyroMag;
      }
      if (gyroMag>SWING_START_GYRO_THRESHOLD) {lastMotionTime = millis();}
    }

    if (impactDetected) {
      noInterrupts();
      impactDetected = false;
      interrupts();

      piezoAnalogValue = analogRead(PIEZO_ANALOG_PIN);
      realStrike = true;
      downswingTime = millis() - swingStartTime - backswingTime;
      finalAngle = faceAngle;

      state = PROCESSING;

      Serial.println("Impact detected.");
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
    Serial.println();
    Serial.println("----- Swing Result -----");

    if (realStrike)
    {
      float tempoRatio = (float)backswingTime / (float)downswingTime;
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