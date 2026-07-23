#include "sensors.h"
#include <Arduino.h>
#include <cmath>

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

// Simulate RPM sensor with realistic engine behavior
float get_rpm() {
  static float rpm = 2000.0;
  static unsigned long state_start_time = 0;
  static int state = 0;  // 0=idle, 1=revving, 2=backing_off

  unsigned long now = millis();
  unsigned long time_in_state = now - state_start_time;

  switch(state) {
    case 0:  // IDLE - bobble around 2000 RPM for ~400ms
      if (time_in_state < random(2000, 5000)) {
        // Small bobble: ±50 RPM around 2000
        rpm = 2000.0 + (sin((float)time_in_state / 100.0) * 100.0);
      } else {
        // Transition to revving
        state = 1;
        state_start_time = now;
        rpm = 2000.0;
      }
      break;

    case 1:  // REVVING UP - accelerate to ~13500 in ~600ms
      if (time_in_state < random(650, 3000)) {
        // Non-linear rev curve (more aggressive initially)
        float progress = (float)time_in_state / 600.0;
        rpm = 2000.0 + (progress * progress * 11500.0);  // Quadratic acceleration
      } else {
        // Transition to backing off
        state = 2;
        state_start_time = now;
        rpm = 13500.0;
      }
      break;

    case 2:  // BACKING OFF - decelerate back to idle in ~500ms
      if (time_in_state < 500) {
        float progress = (float)time_in_state / 500.0;
        rpm = 13500.0 - (progress * progress * 11500.0);  // Quadratic deceleration
      } else {
        // Loop back to idle
        state = 0;
        state_start_time = now;
        rpm = 2000.0;
      }
      break;
  }

  return rpm;
}
