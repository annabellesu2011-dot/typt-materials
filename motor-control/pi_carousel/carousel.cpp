// carousel.cpp — Pi-only magnetic carousel controller (Pi 4 + TMC2209 + 17HS4401)
#include <pigpio.h>      // pigpio library: GPIO, hardware PWM, microsecond timing
#include <cstdio>        // printf, fprintf, fopen for printing and the CSV file
#include <cmath>         // fabs and M_PI for the math
#include <cstdint>       // fixed-size integer types like uint32_t
#include <string>        // std::string for reading command lines
#include <iostream>      // std::getline and std::cin for keyboard input
#include <sstream>       // std::istringstream to split a command into words
#include <thread>        // std::thread so input doesn't block the control loop
#include <mutex>         // std::mutex so the two threads don't clash
#include <atomic>        // std::atomic for values shared with the sensor callback
#include <chrono>        // timing for the 5 ms loop sleep
#include <csignal>       // catching Ctrl+C so we stop the motor safely

const int STEP_PIN = 18;                 // GPIO18 = hardware PWM0, sends step pulses
const int DIR_PIN  = 23;                 // GPIO23 sets direction on the TMC2209
const int EN_PIN   = 24;                 // GPIO24 enables the driver (LOW = on)
const int IR_PIN   = 17;                 // GPIO17 reads the IR sensor mark

const double STEPS_PER_REV = 200.0 * 16; // 200 full steps x 16 microsteps per turn
const double MAX_RPM = 650.0;            // safety limit on speed
const double DEFAULT_ACCEL = 100.0;      // RPM per second used by the S command

std::atomic<uint32_t> lastTick{0};       // time (us) of the last IR mark
std::atomic<uint32_t> periodUs{0};       // time (us) between the last two marks
std::atomic<bool> running{true};         // goes false when we want to quit

std::mutex m;                            // protects the ramp variables below
double currentRpm = 0;                   // speed we are commanding right now
double startRpm = 0;                     // speed when the current ramp began
double targetRpm = 0;                    // speed the ramp is heading to
double rampDurS = 0;                     // how long the ramp lasts, in seconds
double rampStartS = 0;                   // when the ramp started, in seconds

double nowS() { return gpioTick() / 1e6; }  // current time in seconds from pigpio's us clock
// note: gpioTick wraps every ~72 min; fine for normal runs, restart for very long ones

void irCallback(int gpio, int level, uint32_t tick) {  // pigpio calls this on each pin change
  if (level != 0) return;                              // only count the falling edge (mark seen)
  uint32_t dt = tick - lastTick.load();                // time since the previous mark
  if (dt > 2000) {                                     // ignore glitches shorter than 2 ms
    periodUs = dt;                                     // store one-revolution period
    lastTick = tick;                                   // remember this mark's time
  }
}

void setStepRate(double rpm) {                         // turn a speed into a pulse frequency
  double hz = fabs(rpm) / 60.0 * STEPS_PER_REV;        // revs/s x steps/rev = steps/s
  gpioWrite(DIR_PIN, rpm >= 0 ? 1 : 0);                // pick direction from the sign
  if (hz < 1) gpioHardwarePWM(STEP_PIN, 0, 0);         // too slow means stop: PWM off
  else gpioHardwarePWM(STEP_PIN, (unsigned)(hz + 0.5), 500000);  // exact Hz, 50% duty
}

void startRamp(double to, double seconds) {            // begin a new linear ramp
  if (to > MAX_RPM) to = MAX_RPM;                      // clamp to the top limit
  if (to < -MAX_RPM) to = -MAX_RPM;                    // clamp reverse too
  std::lock_guard<std::mutex> lock(m);                 // lock while changing shared values
  startRpm = currentRpm;                               // ramp starts from where we are
  targetRpm = to;                                      // where we want to end up
  rampDurS = seconds;                                  // how long to take
  rampStartS = nowS();                                 // mark the start time
}

double measuredRpm() {                                 // real disc speed from the IR sensor
  uint32_t p = periodUs.load();                        // latest period in us
  if (p == 0 || gpioTick() - lastTick.load() > 2000000) return 0;  // no mark for 2 s = stopped
  return 60e6 / p;                                     // 60,000,000 us per minute / period
}

void inputThread() {                                   // runs separately, waits for typed commands
  std::string line;                                    // holds one typed line
  while (running && std::getline(std::cin, line)) {    // read until quit or end of input
    std::istringstream in(line);                       // split the line into words
    char cmd = 0; double a = 0, b = 0;                 // command letter and two numbers
    in >> cmd >> a >> b;                               // read them (missing ones stay 0)
    cmd = toupper(cmd);                                // accept s or S
    if (cmd == 'S') startRamp(a, fabs(a - currentRpm) / DEFAULT_ACCEL);  // go at default accel
    else if (cmd == 'R') startRamp(a, b);              // ramp to a over b seconds
    else if (cmd == 'X') startRamp(0, 0);              // stop immediately
    else if (cmd == 'Q') running = false;              // quit the program
    else fprintf(stderr, "# unknown command\n");       // anything else is an error
  }
}

void onSignal(int) { running = false; }                // Ctrl+C sets the quit flag

int main(int argc, char** argv) {                      // program starts here
  if (gpioInitialise() < 0) return 1;                  // start pigpio (needs sudo)
  signal(SIGINT, onSignal);                            // catch Ctrl+C after pigpio's own handler
  gpioSetMode(DIR_PIN, PI_OUTPUT);                     // direction pin is an output
  gpioSetMode(EN_PIN, PI_OUTPUT);                      // enable pin is an output
  gpioSetMode(IR_PIN, PI_INPUT);                       // sensor pin is an input
  gpioSetPullUpDown(IR_PIN, PI_PUD_UP);                // pull-up so the pin idles high
  gpioWrite(EN_PIN, 0);                                // switch the driver on
  gpioSetAlertFunc(IR_PIN, irCallback);                // call irCallback on sensor changes

  const char* fname = argc > 1 ? argv[1] : "run.csv";  // CSV name from the command line
  FILE* f = fopen(fname, "w");                         // open the log file
  const char* header = "t_s,set_rpm,meas_rpm,set_rad_s,meas_rad_s\n";  // column names
  fputs(header, f); fputs(header, stdout);             // write header to file and screen

  std::thread input(inputThread);                      // start listening for commands
  input.detach();                                      // let it run on its own
  double t0 = nowS(), lastLog = 0;                     // start time and last log time
  const double k = 2 * M_PI / 60.0;                    // converts RPM to rad/s

  while (running) {                                    // main control loop
    double r;                                          // speed to command this cycle
    {
      std::lock_guard<std::mutex> lock(m);             // read ramp values safely
      double t = nowS() - rampStartS;                  // time into the ramp
      r = (rampDurS <= 0 || t >= rampDurS) ? targetRpm // ramp done: hold target
          : startRpm + (targetRpm - startRpm) * t / rampDurS;  // else interpolate linearly
      if (r != currentRpm) { currentRpm = r; setStepRate(r); } // update only on change
    }
    double t = nowS() - t0;                            // seconds since program start
    if (t - lastLog >= 0.1) {                          // log 10 times per second
      lastLog = t;                                     // remember when we logged
      double meas = measuredRpm();                     // read the real speed
      char buf[128];                                   // text buffer for one line
      snprintf(buf, sizeof buf, "%.2f,%.2f,%.2f,%.3f,%.3f\n",
               t, r, meas, r * k, meas * k);           // format time, set and measured speeds
      fputs(buf, f); fputs(buf, stdout);               // write to file and screen
      fflush(f);                                       // save now in case of a crash
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));  // run every 5 ms
  }

  gpioHardwarePWM(STEP_PIN, 0, 0);                     // stop the step pulses
  gpioWrite(EN_PIN, 1);                                // switch the driver off
  fclose(f);                                           // close the CSV
  gpioTerminate();                                     // release pigpio
  return 0;                                            // done
}
