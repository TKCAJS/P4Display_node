#include "sensors.h"
#include <Arduino.h>
#include <cmath>

// Engine rev range the simulator sweeps. RPM_PEAK is the hard ceiling — it
// matches the Screen1 slider range and Screen6's REV_MAX_RPM, so a full rev
// pins the gauges rather than running off the end of them.
static const float RPM_IDLE = 2000.0f;
static const float RPM_PEAK = 14000.0f;

// Triangle sweep between lo and hi at `per_s` units/second, derived purely from
// millis() so the value depends on elapsed time, not on how often it is read.
//
// (The earlier simulators stepped a static by 3 units *per call*. canGetSnapshot()
// is called several times per loop() — heartbeat, gauges, and each SDLogger
// getter — so the sweep advanced hundreds of degrees a second and whatever the
// cooling page sampled at its 500 ms tick was effectively random: the arcs and
// labels jumped instead of ramping.)
// `phase_s` shifts a sweep along its own cycle so signals sharing a range don't
// all move in lockstep.
static float sweep(float lo, float hi, float per_s, float phase_s) {
  const float span   = hi - lo;
  const float period = 2.0f * span / per_s;   // seconds for one up+down cycle
  const float t      = fmodf((float)millis() / 1000.0f + phase_s, period) * per_s;
  return (t <= span) ? lo + t : hi - (t - span);
}

// All three sweep at 2 units/s, i.e. one whole degree or percent per 500 ms
// cooling-page update — the finest step the display can show.
float get_temperature() {
  return sweep(20.0f, 80.0f, 2.0f, 0.0f);
}

float get_fuel_level() {
  return sweep(0.0f, 100.0f, 2.0f, 0.0f);
}

float get_pump_duty() {
  return sweep(0.0f, 100.0f, 2.0f, 37.0f);
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
