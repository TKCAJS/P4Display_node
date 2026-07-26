#ifndef DISPLAY_CAN_H
#define DISPLAY_CAN_H

/*
 * STUB DisplayCan for the P4 UI port (phase 3) — same API surface as the real
 * esp32_T89Display/src/DisplayCan.h, but no TWAI: the snapshot is fed from the
 * sensors.h simulators so the gauges animate without a CAN transceiver wired.
 * The real DisplayCan drops back in once CAN TX/RX pins are wired to JP1.
 */

#include <Arduino.h>
#include "can_ids.h"
#include "sensors.h"

struct CanSnapshot {
    uint8_t  gear;
    uint16_t rpm;
    float    engineTemp;
    float    oilTemp;
    float    radiatorTemp;
    float    radOutTemp;
    uint8_t  warningFlags;
    uint8_t  shiftMode;
    uint8_t  stackTarget;
    bool     valid;
    uint16_t versionMain;
    uint16_t versionRear;
    uint8_t  pumpDuty;
};

inline uint8_t& _pumpTarget() { static uint8_t t = 82; return t; }

inline CanSnapshot canGetSnapshot() {
    CanSnapshot s = {};

    // Gear cycles N,1..6 every 2 s; a stacked downshift shows briefly in 5th/6th
    uint8_t phase = (millis() / 2000) % 7;
    s.gear        = (phase == 0) ? GEAR_NEUTRAL : phase;
    s.stackTarget = (phase >= 5 && (millis() / 500) % 4 == 0) ? 2 : 0;

    s.rpm          = (uint16_t)get_rpm();
    s.radiatorTemp = get_temperature();            // 20..80 sweep
    s.engineTemp   = s.radiatorTemp + 6.0f;
    s.radOutTemp   = s.radiatorTemp - 9.0f;
    s.oilTemp      = get_fuel_level();             // reused sweep for the oil bar
    // Pump duty ramps 1 % per cooling-page update and reverses at the ends,
    // rather than snapping 100 -> 0 like the old sawtooth.
    s.pumpDuty     = (uint8_t)get_pump_duty();
    s.shiftMode    = 0;
    s.warningFlags = 0;
    s.versionMain  = 104;
    s.versionRear  = 102;
    s.valid        = true;
    return s;
}

inline void    canBegin()  {}
inline void    canUpdate() {}
inline bool    canIsValid()                    { return true; }
inline void    canSetPumpTarget(uint8_t degC)  { _pumpTarget() = degC; }
inline uint8_t canGetPumpTarget()              { return _pumpTarget(); }

// Getters used by SDLogger — derived from the same simulated snapshot.
// GPS/IMU report "no data" (empty CSV cells), like a car without the GPS node.
inline uint8_t  canGetGear()         { return canGetSnapshot().gear; }
inline uint16_t canGetRPM()          { return canGetSnapshot().rpm; }
inline float    canGetEngineTemp()   { return canGetSnapshot().engineTemp; }
inline float    canGetOilTemp()      { return canGetSnapshot().oilTemp; }
inline uint8_t  canGetWarningFlags() { return 0; }
inline uint8_t  canGetShiftMode()    { return 0; }
inline bool     canGetGpsLocation(int32_t*, int32_t*) { return false; }
inline float    canGetGpsMph()       { return -1.0f; }
inline uint8_t  canGetGpsSats()      { return 0; }
inline bool     canGetGForce(float*, float*) { return false; }

#endif // DISPLAY_CAN_H
