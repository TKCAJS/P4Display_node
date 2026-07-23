#include "SDLogger.h"

static bool sdAvailable = false;
static File logFile;
static String logFileName;
static unsigned long sessionStartTime = 0;
static unsigned long lastLogTime = 0;
static uint32_t recordCount = 0;

static String logBuffer = "";
static uint16_t bufferRecords = 0;
static unsigned long lastFlushTime = 0;
static bool paused = false;

static bool openNewSession() {
    int sessionNum = 0;
    do {
        sessionNum++;
        logFileName = "/logs/session_" + String(sessionNum) + ".csv";
    } while (SD_MMC.exists(logFileName.c_str()) && sessionNum < 9999);

    Serial.printf("Creating: %s\n", logFileName.c_str());

    logFile = SD_MMC.open(logFileName.c_str(), FILE_WRITE);
    if (!logFile) {
        Serial.println("ERROR: Failed to create log file");
        return false;
    }

    logFile.println("SessionTime_ms,SystemTime_ms,Gear,RPM,EngineTemp_C,OilTemp_C,WarningFlags,ShiftMode,Lat,Lon,Sats,GpsMph,LatG,FwdG");
    logFile.flush();

    sessionStartTime = millis();
    recordCount = 0;
    return true;
}

static bool mountCard() {
    // TF_VCC (LDO channel 4) is already acquired and held by the display BSP
    // (board_p4_display_init), so the channel is busy — SD_MMC.begin() must
    // NOT create its own LDO driver. -1 = "externally powered", skips it.
    SD_MMC.setPowerChannel(-1);
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3);

    // Cards left mid-transaction by a reset can fail the first CMD0 — retry
    bool mounted = false;
    for (int attempt = 0; attempt < 3 && !mounted; attempt++) {
        if (attempt) {
            SD_MMC.end();
            delay(250);
        }
        mounted = SD_MMC.begin("/sdcard", false /* 4-bit */);
    }
    if (!mounted) {
        Serial.println("ERROR: SD card failed");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("ERROR: No SD card");
        return false;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) Serial.println("MMC");
    else if (cardType == CARD_SD) Serial.println("SDSC");
    else if (cardType == CARD_SDHC) Serial.println("SDHC");

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    if (!SD_MMC.exists("/logs")) {
        SD_MMC.mkdir("/logs");
    }

    return true;
}

bool sdLoggerBegin() {
    Serial.println("\n=== SD Logger Initialization ===");

    if (!mountCard() || !openNewSession()) {
        return false;
    }

    sdAvailable = true;
    Serial.println("SD logger ready");

    return true;
}

bool sdLoggerRemount() {
    if (sdAvailable) return true;

    Serial.println("SD: retrying mount");
    if (!mountCard()) return false;

    // No session file here: while paused (pit mode) none is wanted, and
    // sdLoggerResume() opens one when logging restarts.
    sdAvailable = true;
    Serial.println("SD: remount OK");
    return true;
}

void sdLoggerLog() {
    if (!sdAvailable || paused || !canIsValid()) return;

    unsigned long currentTime = millis();
    if (currentTime - lastLogTime < LOG_INTERVAL_MS) return;
    lastLogTime = currentTime;

    unsigned long sessionTime = currentTime - sessionStartTime;

    char logLine[224];
    int n = snprintf(logLine, sizeof(logLine),
             "%lu,%lu,%d,%u,%.1f,%.1f,%u,%u,",
             sessionTime,
             currentTime,
             canGetGear(),
             canGetRPM(),
             canGetEngineTemp(),
             canGetOilTemp(),
             canGetWarningFlags(),
             canGetShiftMode());

    // GPS/IMU columns — empty cells when the data is stale or there's no fix
    int32_t latE7, lngE7;
    if (canGetGpsLocation(&latE7, &lngE7))
        n += snprintf(logLine + n, sizeof(logLine) - n, "%.7f,%.7f,%u,",
                      latE7 * 1e-7, lngE7 * 1e-7, canGetGpsSats());
    else
        n += snprintf(logLine + n, sizeof(logLine) - n, ",,%u,", canGetGpsSats());

    float mph = canGetGpsMph();
    if (mph >= 0.0f)
        n += snprintf(logLine + n, sizeof(logLine) - n, "%.1f,", mph);
    else
        n += snprintf(logLine + n, sizeof(logLine) - n, ",");

    float latG, fwdG;
    if (canGetGForce(&latG, &fwdG))
        snprintf(logLine + n, sizeof(logLine) - n, "%.3f,%.3f\n", latG, fwdG);
    else
        snprintf(logLine + n, sizeof(logLine) - n, ",\n");

    logBuffer += String(logLine);
    bufferRecords++;
    recordCount++;

    if (bufferRecords >= LOG_BUFFER_SIZE) {
        sdLoggerFlush();
    }
}

void sdLoggerFlush() {
    if (!sdAvailable || logBuffer.length() == 0) return;

    if (!logFile) {
        logFile = SD_MMC.open(logFileName.c_str(), FILE_APPEND);
        if (!logFile) {
            Serial.println("ERROR: Failed to reopen log file");
            sdAvailable = false;
            return;
        }
    }

    logFile.print(logBuffer);
    logFile.flush();

    logBuffer = "";
    bufferRecords = 0;
    lastFlushTime = millis();
}

void sdLoggerUpdate() {
    if (paused) return;
    sdLoggerLog();
    if (sdAvailable && (millis() - lastFlushTime >= 1000)) {
        sdLoggerFlush();
    }
}

void sdLoggerPause() {
    if (paused) return;
    paused = true;
    if (sdAvailable) {
        sdLoggerFlush();
        if (logFile) {
            logFile.close();
            Serial.printf("Log paused: %s (%lu records)\n", logFileName.c_str(), recordCount);
        }
    }
}

void sdLoggerResume() {
    if (!paused) return;
    paused = false;
    if (sdAvailable && !openNewSession()) {
        sdAvailable = false;
    }
}

void sdLoggerClose() {
    if (sdAvailable) {
        sdLoggerFlush();
        if (logFile) {
            logFile.close();
            Serial.printf("Log closed. Records: %lu\n", recordCount);
        }
    }
}

bool sdLoggerIsAvailable() {
    return sdAvailable;
}

uint32_t sdLoggerGetRecordCount() {
    return recordCount;
}

String sdLoggerGetFileName() {
    return logFileName;
}
