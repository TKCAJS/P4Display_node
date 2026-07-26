#ifndef SENSORS_H
#define SENSORS_H

// Get simulated sensor values. All are pure functions of millis(), so calling
// them repeatedly within one frame yields the same value.
float get_temperature();
float get_fuel_level();
float get_pump_duty();
float get_rpm();

#endif
