#ifndef GAUGE_UPDATER_H
#define GAUGE_UPDATER_H

#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"
#include "DisplayCan.h"
#include "sensors.h"

// Update intervals in milliseconds
#define TEMP_UPDATE_MS 500
#define FUEL_UPDATE_MS 600
#define RPM_UPDATE_MS 30

// Simple function - no class needed
void updateGauges();

#endif // GAUGE_UPDATER_H

// end of code