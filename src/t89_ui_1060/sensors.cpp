#include "sensors.h"
#include <Arduino.h>
#include <cmath>

static const float RPM_IDLE = 2000.0f;
static const float RPM_PEAK = 14000.0f;   // ui_gaugerpm's full scale, and a hard ceiling

// Triangle wave between lo and hi at `per_s` units/second, a pure function of
// millis(). These must not step a static per call: canGetSnapshot() is read
// several times per loop() (heartbeat, gauges, and each SDLogger getter), so a
// per-call simulator advances at loop rate rather than wall-clock rate — at a
// ~5 ms loop that is hundreds of units a second, bouncing off both limits
// between two 500 ms cooling-page updates, and whatever Screen4 sampled looked
// random. `phase_s` offsets a sweep so signals sharing a range don't move in
// lockstep.
static float sweep(float lo, float hi, float per_s, float phase_s) {
  const float span   = hi - lo;
  const float period = 2.0f * span / per_s;   // seconds for one up+down cycle
  const float t      = fmodf((float)millis() / 1000.0f + phase_s, period) * per_s;
  return (t <= span) ? lo + t : hi - (t - span);
}

// All three sweep at 2 units/s, i.e. exactly one whole degree or percent per
// TEMP_UPDATE_MS (500 ms) cooling-page tick — the finest step the arcs and
// DSEG48 labels can show.
float get_temperature() {
  return sweep(20.0f, 80.0f, 2.0f, 0.0f);
}

float get_fuel_level() {
  return sweep(0.0f, 100.0f, 2.0f, 0.0f);
}

float get_pump_duty() {
  return sweep(0.0f, 100.0f, 2.0f, 37.0f);
}

// Simulated engine: idle bobble -> pull to the limiter -> back off, repeat.
//
// Each state latches its duration on entry and normalises progress over that
// same duration, so a ramp lands exactly on its end value. (Re-rolling random()
// on every call while dividing progress by a fixed 600 ms let the quadratic
// keep climbing once a rev outlasted that, briefly reporting over 30,000 RPM
// and wrapping the uint16_t in the snapshot.)
float get_rpm() {
  static unsigned long state_start = 0;
  static unsigned long state_ms    = 3000;   // latched once per state
  static int           state       = 0;      // 0=idle, 1=revving, 2=backing off

  const unsigned long now  = millis();
  const unsigned long t    = now - state_start;
  const float         prog = state_ms ? (float)t / (float)state_ms : 1.0f;

  float rpm = RPM_IDLE;

  switch (state) {
    case 0:  // IDLE - bobble around idle
      rpm = RPM_IDLE + sinf((float)t / 100.0f) * 100.0f;
      if (t >= state_ms) {
        state = 1; state_start = now; state_ms = random(650, 3000);
      }
      break;

    case 1:  // REVVING UP - quadratic pull to the limiter
      rpm = RPM_IDLE + (RPM_PEAK - RPM_IDLE) * prog * prog;
      if (t >= state_ms) {
        rpm = RPM_PEAK;
        state = 2; state_start = now; state_ms = 500;
      }
      break;

    case 2:  // BACKING OFF - quadratic drop back to idle
      rpm = RPM_PEAK - (RPM_PEAK - RPM_IDLE) * prog * prog;
      if (t >= state_ms) {
        rpm = RPM_IDLE;
        state = 0; state_start = now; state_ms = random(2000, 5000);
      }
      break;
  }

  if (rpm < 0.0f)     rpm = 0.0f;
  if (rpm > RPM_PEAK) rpm = RPM_PEAK;
  return rpm;
}
