#include <SoftwareSerial.h>

// ===== PIN CONNECTIONS =====
const int ENA = 5;          // L298N ENA (remove ENA jumper for PWM)
const int IN1 = 7;          // L298N IN1
const int IN2 = 8;          // L298N IN2
const int HALL_PIN = 2;     // Hall sensor OUT
const int LED_PIN = 12;     // LED through 220-330 ohm resistor
const int BUZZER_PIN = 11;  // Buzzer
const int RESET_PIN = 4;    // Reset button to GND

// Arduino Uno to NodeMCU serial connection
// Uno D9  = RX, receives from NodeMCU TX
// Uno D10 = TX, sends to NodeMCU RX
SoftwareSerial espSerial(9, 10);

// ===== MOTOR SETTINGS =====
int motorSpeed = 200;       // PWM speed: 0-255
const int MIN_SPEED = 0;
const int MAX_SPEED = 255;

// Set this to the number of Hall pulses per motor revolution
const int PULSES_PER_REV = 1;

// Alarm threshold
const float LOW_RPM_THRESHOLD = 2100.0;

// ===== TIMING SETTINGS =====
const unsigned long RPM_INTERVAL = 500;
const unsigned long STARTUP_GRACE = 3000;
const unsigned long STALL_TIMEOUT = 1500;
const unsigned long ALARM_INTERVAL = 300;

// ===== SENSOR AND STATUS VARIABLES =====
volatile unsigned long hallPulses = 0;
volatile unsigned long lastPulseTime = 0;

unsigned long lastRpmUpdate = 0;
unsigned long lastAlarmToggle = 0;
unsigned long motorStartTime = 0;

float rpm = 0;

bool fault = false;
bool manualStop = false;
bool motorRunning = false;
bool alarmState = false;

String serialCommand = "";

// ===== HALL SENSOR INTERRUPT =====
void hallSensorISR() {
  hallPulses++;
  lastPulseTime = millis();
}

// ===== TURN OFF ALL ALARMS =====
void stopAlarm() {
  alarmState = false;
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  lastAlarmToggle = millis();
}

// ===== STOP MOTOR =====
void stopMotor() {
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  motorRunning = false;
  rpm = 0;
}

// ===== START MOTOR =====
void startMotor() {
  fault = false;
  manualStop = false;

  stopAlarm();

  noInterrupts();
  hallPulses = 0;
  lastPulseTime = 0;
  interrupts();

  rpm = 0;
  motorStartTime = millis();
  lastRpmUpdate = millis();
  motorRunning = true;

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, motorSpeed);

  Serial.print("Motor STARTED. PWM = ");
  Serial.println(motorSpeed);
}

// ===== CHANGE MOTOR SPEED =====
void changeMotorSpeed(int requestedSpeed) {
  if (requestedSpeed < MIN_SPEED ||
      requestedSpeed > MAX_SPEED) {
    Serial.println("Enter a PWM value from 0 to 255.");
    return;
  }

  if (fault) {
    Serial.println("Fault active. Type RESET before changing speed.");
    return;
  }

  motorSpeed = requestedSpeed;

  if (requestedSpeed == 0) {
    stopMotor();
    manualStop = true;
    stopAlarm();
    Serial.println("Motor stopped because PWM is 0.");
    return;
  }

  if (manualStop || !motorRunning) {
    startMotor();
  } else {
    analogWrite(ENA, motorSpeed);
  }

  Serial.print("PWM set to: ");
  Serial.println(motorSpeed);
}

// ===== REGISTER A MOTOR FAULT =====
void triggerFault(const char* reason) {
  if (!fault && !manualStop) {
    fault = true;
    stopMotor();

    Serial.print("FAULT: ");
    Serial.println(reason);
  }
}

// ===== LED AND BUZZER ALARM LOGIC =====
void updateAlarm() {
  unsigned long now = millis();

  // FAULT: LED and buzzer blink together
  if (fault && !manualStop) {
    if (now - lastAlarmToggle >= ALARM_INTERVAL) {
      lastAlarmToggle = now;
      alarmState = !alarmState;

      digitalWrite(LED_PIN, alarmState ? HIGH : LOW);
      digitalWrite(BUZZER_PIN, alarmState ? HIGH : LOW);
    }
  }

  // LOW RPM: only the LED blinks
  else if (motorRunning &&
           !manualStop &&
           now - motorStartTime >= STARTUP_GRACE &&
           rpm < LOW_RPM_THRESHOLD) {

    digitalWrite(BUZZER_PIN, LOW);

    if (now - lastAlarmToggle >= ALARM_INTERVAL) {
      lastAlarmToggle = now;
      alarmState = !alarmState;
      digitalWrite(LED_PIN, alarmState ? HIGH : LOW);
    }
  }

  // NORMAL OR STOPPED: both remain OFF
  else {
    alarmState = false;
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    lastAlarmToggle = now;
  }
}

// ===== RESET FAULT =====
void resetFault() {
  Serial.println("Fault RESET. Restarting motor.");
  startMotor();
}

// ===== PROCESS SERIAL COMMANDS =====
void processCommand(String command) {
  command.trim();
  command.toUpperCase();

  if (command == "STOP") {
    manualStop = true;
    fault = false;

    stopMotor();
    stopAlarm();

    noInterrupts();
    hallPulses = 0;
    lastPulseTime = 0;
    interrupts();

    Serial.println("Motor STOPPED. Alarm OFF.");
  }

  else if (command == "START") {
    if (motorRunning && !fault && !manualStop) {
      Serial.println("Motor is already running.");
    } else {
      startMotor();
    }
  }

  else if (command == "RESET") {
    resetFault();
  }

  else {
    bool isNumber = command.length() > 0;

    for (unsigned int i = 0; i < command.length(); i++) {
      if (!isDigit(command[i])) {
        isNumber = false;
        break;
      }
    }

    if (isNumber) {
      int requestedSpeed = command.toInt();
      changeMotorSpeed(requestedSpeed);
    }

    else if (command.length() > 0) {
      Serial.println("Commands: 0-255, START, STOP, RESET");
    }
  }
}

// ===== READ SERIAL MONITOR COMMANDS =====
void checkSerialCommands() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialCommand.length() > 0) {
        processCommand(serialCommand);
        serialCommand = "";
      }
    }

    else if (serialCommand.length() < 30) {
      serialCommand += c;
    }

    else {
      serialCommand = "";
    }
  }
}

// ===== CALCULATE RPM =====
void updateRPM() {
  unsigned long now = millis();

  if (now - lastRpmUpdate >= RPM_INTERVAL) {
    unsigned long elapsed = now - lastRpmUpdate;
    lastRpmUpdate = now;

    noInterrupts();
    unsigned long pulses = hallPulses;
    hallPulses = 0;
    interrupts();

    if (motorRunning && !fault && !manualStop) {
      rpm = (pulses * 60000.0) /
            (elapsed * PULSES_PER_REV);
    } else {
      rpm = 0;
    }
  }
}

// ===== DETECT MOTOR STALL =====
void checkMotorFault() {
  if (!motorRunning || fault || manualStop) {
    return;
  }

  unsigned long now = millis();

  // Give the motor time to start
  if (now - motorStartTime < STARTUP_GRACE) {
    return;
  }

  unsigned long pulseTime;

  noInterrupts();
  pulseTime = lastPulseTime;
  interrupts();

  // No Hall pulses means the motor may be stalled
  if (pulseTime == 0 ||
      now - pulseTime >= STALL_TIMEOUT) {
    triggerFault("No Hall sensor pulses detected");
  }
}

// ===== SEND DATA TO NODEMCU DASHBOARD =====
void sendDashboardData() {
  const char* status;

  if (fault) {
    status = "FAULT";
  } else if (manualStop) {
    status = "STOPPED";
  } else if (motorRunning) {
    status = "RUNNING";
  } else {
    status = "STOPPED";
  }

  espSerial.print("{\"rpm\":");
  espSerial.print((int)rpm);
  espSerial.print(",\"status\":\"");
  espSerial.print(status);
  espSerial.println("\"}");
}

// ===== SETUP =====
void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(HALL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  espSerial.begin(9600);

  attachInterrupt(
    digitalPinToInterrupt(HALL_PIN),
    hallSensorISR,
    FALLING
  );

  lastRpmUpdate = millis();

  stopAlarm();
  stopMotor();

  Serial.println("===== JamGuard Motor Protection =====");
  Serial.println("Enter PWM 0-255 to set motor speed.");
  Serial.println("Commands: START, STOP, RESET");
  Serial.println("Low RPM threshold: 2100 RPM");
  Serial.println("Low RPM: LED blinks only.");
  Serial.println("Fault: LED and buzzer blink together.");

  startMotor();
}

// ===== MAIN LOOP =====
void loop() {
  checkSerialCommands();

  // Physical reset button: D4 connected to GND
  static bool previousButtonState = HIGH;
  static unsigned long lastButtonChange = 0;

  bool currentButtonState = digitalRead(RESET_PIN);

  if (currentButtonState != previousButtonState) {
    lastButtonChange = millis();
    previousButtonState = currentButtonState;
  }

  if (currentButtonState == LOW &&
      millis() - lastButtonChange >= 30 &&
      fault) {
    resetFault();
  }

  updateRPM();
  checkMotorFault();
  updateAlarm();

  // Send RPM and status to NodeMCU every 500 ms
  static unsigned long lastDashboardSend = 0;

  if (millis() - lastDashboardSend >= 500) {
    lastDashboardSend = millis();

    sendDashboardData();

    Serial.print("RPM: ");
    Serial.print((int)rpm);

    Serial.print(" | PWM: ");
    Serial.print(motorSpeed);

    Serial.print(" | Status: ");

    if (fault) {
      Serial.println("FAULT");
    }

    else if (manualStop) {
      Serial.println("STOPPED");
    }

    else if (motorRunning &&
             millis() - motorStartTime >= STARTUP_GRACE &&
             rpm < LOW_RPM_THRESHOLD) {
      Serial.println("LOW RPM - LED ALARM");
    }

    else if (motorRunning) {
      Serial.println("RUNNING");
    }

    else {
      Serial.println("STOPPED");
    }
  }
}
