// Magnetic carousel speed controller
// Arduino Nano/Uno + TMC2209 + 17HS4401, IR sensor reads 1 mark per revolution
#include <Arduino.h>

const uint8_t STEP_PIN = 9;   // must be pin 9 (Timer1 OC1A hardware output)
const uint8_t DIR_PIN  = 8;
const uint8_t EN_PIN   = 7;   // TMC2209 EN, LOW = motor on
const uint8_t IR_PIN   = 2;   // IR sensor output (interrupt pin)

const float STEPS_PER_REV = 200.0 * 16;  // 1.8 deg motor, 16x microstepping (MS1=MS2=HIGH)
const float MAX_RPM = 650.0;
const float DEFAULT_ACCEL = 100.0;       // RPM per second for the S command

volatile uint32_t lastPulseUs = 0, periodUs = 0;

float currentRpm = 0, startRpm = 0, targetRpm = 0;
uint32_t rampStartMs = 0, rampDurMs = 0, lastLogMs = 0;

void irISR() {
  uint32_t now = micros();
  uint32_t dt = now - lastPulseUs;
  if (dt > 2000) { periodUs = dt; lastPulseUs = now; }  // ignore bounce
}

// Hardware step pulses on pin 9 using Timer1 in CTC toggle mode
void setStepRate(float rpm) {
  float hz = rpm / 60.0 * STEPS_PER_REV;
  if (hz < 16) {                          // effectively stopped
    TCCR1A = 0; TCCR1B = 0; digitalWrite(STEP_PIN, LOW);
    return;
  }
  uint8_t presc; float tick;
  if (hz >= 130) { presc = _BV(CS10); tick = 16e6; }  // prescaler 1
  else           { presc = _BV(CS11); tick = 2e6;  }  // prescaler 8
  uint32_t top = (uint32_t)(tick / (2.0 * hz) + 0.5) - 1;
  if (top > 65535) top = 65535;
  noInterrupts();
  TCCR1A = _BV(COM1A0);           // toggle pin 9 on compare match
  TCCR1B = _BV(WGM12) | presc;    // CTC mode
  OCR1A = top;
  if (TCNT1 > top) TCNT1 = 0;     // avoid a long missed cycle
  interrupts();
}

void startRamp(float toRpm, float seconds) {
  toRpm = constrain(toRpm, -MAX_RPM, MAX_RPM);
  startRpm = currentRpm;
  targetRpm = toRpm;
  rampDurMs = (uint32_t)(seconds * 1000.0);
  rampStartMs = millis();
}

void updateRamp() {
  uint32_t t = millis() - rampStartMs;
  float r = (rampDurMs == 0 || t >= rampDurMs)
              ? targetRpm
              : startRpm + (targetRpm - startRpm) * (float)t / rampDurMs;
  if (r != currentRpm) {
    currentRpm = r;
    digitalWrite(DIR_PIN, r >= 0 ? HIGH : LOW);
    setStepRate(fabs(r));
  }
}

float measuredRpm() {
  noInterrupts();
  uint32_t p = periodUs, last = lastPulseUs;
  interrupts();
  if (p == 0 || micros() - last > 2000000UL) return 0;  // no mark for 2 s
  return 60e6 / p;
}

// Commands (one per line):
//   S <rpm>          go to rpm at the default acceleration
//   R <rpm> <sec>    ramp linearly to rpm over sec seconds
//   X                stop now
void readCommand() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  char cmd = toupper(line.charAt(0));
  float a = 0, b = 0;
  int sp = line.indexOf(' ');
  if (sp > 0) {
    String rest = line.substring(sp + 1);
    a = rest.toFloat();
    int sp2 = rest.indexOf(' ');
    if (sp2 > 0) b = rest.substring(sp2 + 1).toFloat();
  }
  if (cmd == 'S')      startRamp(a, fabs(a - currentRpm) / DEFAULT_ACCEL);
  else if (cmd == 'R') startRamp(a, b);
  else if (cmd == 'X') startRamp(0, 0);
  else { Serial.println(F("# unknown command")); return; }
  Serial.print(F("# ok ")); Serial.println(line);
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT_PULLUP);
  digitalWrite(EN_PIN, LOW);
  attachInterrupt(digitalPinToInterrupt(IR_PIN), irISR, FALLING);
  Serial.begin(115200);
  Serial.setTimeout(20);
  Serial.println(F("ms,set_rpm,meas_rpm,set_rad_s,meas_rad_s"));
}

void loop() {
  readCommand();
  updateRamp();
  if (millis() - lastLogMs >= 100) {      // log at 10 Hz
    lastLogMs = millis();
    float m = measuredRpm();
    const float k = 2 * PI / 60.0;
    Serial.print(lastLogMs); Serial.print(',');
    Serial.print(currentRpm, 2); Serial.print(',');
    Serial.print(m, 2); Serial.print(',');
    Serial.print(currentRpm * k, 3); Serial.print(',');
    Serial.println(m * k, 3);
  }
}
