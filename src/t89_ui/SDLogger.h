#ifndef SD_LOGGER_H
#define SD_LOGGER_H

// P4 port of the T89 SDLogger: same API, but the JC4880P443C's card slot is on
// SDMMC (4-bit) instead of SPI, and the card's 3.3 V (TF_VCC) comes from the
// P4's on-chip LDO channel 4 — mountCard() acquires it before SD_MMC.begin().

#include <Arduino.h>
#include <SD_MMC.h>
#include "DisplayCan.h"

// SDMMC pins (JC4880P443C schematic sheet 01; 5.1k pull-ups on board)
#define SD_CLK 43
#define SD_CMD 44
#define SD_D0  39
#define SD_D1  40
#define SD_D2  41
#define SD_D3  42

// TF_VCC: on-chip LDO channel 4 (voltage is fixed at 3.3 V by SD_MMC's driver)
#define SD_LDO_CHAN 4

// Logging config
#define LOG_BUFFER_SIZE 50
#define LOG_INTERVAL_MS 20

// Initialize SD logger
bool sdLoggerBegin();

// Log current CAN data (call from sdLoggerUpdate)
void sdLoggerLog();

// Update logger (call in loop)
void sdLoggerUpdate();

// Flush buffer to SD
void sdLoggerFlush();

// Close log file
void sdLoggerClose();

// Pause logging (pit mode): flush + close the current session file so the
// files on card are complete for download. Resume opens a new session file.
void sdLoggerPause();
void sdLoggerResume();

// Retry mounting a card that failed at boot (no session file is opened —
// that happens on resume). Returns true if the card is mounted.
bool sdLoggerRemount();

// Status
bool sdLoggerIsAvailable();
uint32_t sdLoggerGetRecordCount();
String sdLoggerGetFileName();

#endif // SD_LOGGER_H
