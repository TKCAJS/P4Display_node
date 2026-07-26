#include "sensors.h"
#include <Arduino.h>
#include <cmath>

// Engine rev range the simulator sweeps. RPM_PEAK is the hard ceiling — it
// matches the Screen1 slider range and Screen6's REV_MAX_RPM, so a full rev
// pins the gauges rather than running off the end of them.
static const float RPM_IDLE = 2000.0f;
static const float RPM_PEAK = 14000.0f;

// Simulate temperature sensor (0-100°C range)
float get_temperature() {
  static float temp = 50.0;
  static int direction = 1;

  // Mechanical rise and fall (0.05°C per call = ~1.5°C/sec at 30fps)
  temp += (direction * 3);

  // Bounce at limits
  if (temp >= 80) direction = -1;
  if (temp <= 20) direction = 1;

  return temp;
}

// Simulate fuel level sensor (0-100% range)
float get_fuel_level() {
  static float fuel = 75.0;

  // Slow mechanical decrease (0.02% per call = ~0.6%/sec at 30fps)
  fuel -= 3;

  // Reset when empty
  if (fuel < 0) fuel = 100;

  return fuel;
}

// Simulate RPM sensor with realistic engine behavior.
//
// Each state latches its duration on entry and normalises progress over that
// same duration, so a ramp lands exactly on its end value. (The earlier version
// re-rolled random() on every call while dividing by a fixed 600 ms, so once a
// rev lasted longer than that the quadratic ran away — briefly reporting over
// 30,000 RPM.)
float get_rpm() {
  static float         rpm          = RPM_IDLE;
  static unsigned long state_start  = 0;
  static unsigned long state_len_ms = 2000;   // latched once per state
  static int           state        = 0;      // 0=idle, 1=revving, 2=backing off

  const unsigned long now  = millis();
  const unsigned long t    = now - state_start;
  const float         prog = state_len_ms ? (float)t / (float)state_len_ms : 1.0f;

  switch (state) {
    case 0:  // IDLE - bobble around idle
      rpm = RPM_IDLE + (sinf((float)t / 100.0f) * 100.0f);
      if (t >= state_len_ms) {
        state = 1; state_start = now; state_len_ms = random(650, 3000);
      }
      break;

    case 1:  // REVVING UP - quadratic pull to the limiter
      rpm = RPM_IDLE + (RPM_PEAK - RPM_IDLE) * prog * prog;
      if (t >= state_len_ms) {
        rpm = RPM_PEAK;
        state = 2; state_start = now; state_len_ms = 500;
      }
      break;

    case 2:  // BACKING OFF - quadratic drop back to idle
      rpm = RPM_PEAK - (RPM_PEAK - RPM_IDLE) * prog * prog;
      if (t >= state_len_ms) {
        rpm = RPM_IDLE;
        state = 0; state_start = now; state_len_ms = random(2000, 5000);
      }
      break;
  }

  if (rpm < 0.0f)     rpm = 0.0f;
  if (rpm > RPM_PEAK) rpm = RPM_PEAK;
  return rpm;
}
