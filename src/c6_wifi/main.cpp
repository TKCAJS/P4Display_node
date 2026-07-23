/*
 * c6_wifi — ESP32-C6 co-processor diagnostic + firmware updater + SoftAP test.
 *
 * The Guition JC4880P443C pairs the P4 with an ESP32-C6 over SDIO (ESP-Hosted).
 * The factory C6 slave firmware is usually older than what the Arduino core's
 * host stack expects, so WiFi fails until the slave is updated. The stock
 * ESP_HostedOTA example needs working WiFi to download the update — a catch-22.
 * This sketch instead embeds the framework's own matching slave binary
 * (framework-arduinoespressif32-libs/hosted/esp32c6-v2.9.6.bin) and pushes it
 * over the SDIO link, no network needed.
 *
 * Boot flow:
 *   1. init hosted (WiFi.STA.begin()), print host + slave versions
 *   2. if the host stack wants a different slave version -> flash the embedded
 *      image (Begin/Write/End/Activate, per ESP_HostedOTA.cpp) and restart
 *   3. otherwise -> SoftAP test with the PitServer config (T89-Pits-style)
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp32-hal-hosted.h"

extern const uint8_t c6fw_start[] asm("_binary_src_c6_wifi_c6fw_bin_start");
extern const uint8_t c6fw_end[] asm("_binary_src_c6_wifi_c6fw_bin_end");

#define AP_SSID "T89-P4-Test"
#define AP_PASS "t89pit123"

static bool updateSlaveFromEmbedded(void) {
    const uint8_t *p   = c6fw_start;
    size_t         len = (size_t)(c6fw_end - c6fw_start);
    Serial.printf("[C6] flashing embedded slave image, %u bytes\n", (unsigned)len);

    if (!hostedBeginUpdate()) {
        Serial.println("[C6] ERROR: begin update failed");
        return false;
    }
    size_t done = 0;
    while (done < len) {
        size_t chunk = min((size_t)2048, len - done);
        if (!hostedWriteUpdate((uint8_t *)(p + done), chunk)) {
            Serial.printf("[C6] ERROR: write failed at %u/%u\n", (unsigned)done, (unsigned)len);
            return false;
        }
        done += chunk;
        if (done % (128 * 1024) < 2048) {
            Serial.printf("[C6] ... %u/%u\n", (unsigned)done, (unsigned)len);
        }
    }
    if (!hostedEndUpdate()) {
        Serial.println("[C6] ERROR: end update failed");
        return false;
    }
    if (!hostedActivateUpdate()) {
        Serial.println("[C6] ERROR: activate failed");
        return false;
    }
    Serial.println("[C6] SUCCESS: slave updated — restarting host");
    return true;
}

void setup(void) {
    Serial.begin(115200);
    delay(2000);   // let USB CDC enumerate so early prints are visible
    Serial.println("\n[C6] ESP32-C6 hosted WiFi diagnostic");

    uint32_t maj, min_, pat;
    hostedGetHostVersion(&maj, &min_, &pat);
    Serial.printf("[C6] host stack expects: %lu.%lu.%lu\n", maj, min_, pat);

    // Init the hosted link (this is what first talks to the C6 over SDIO).
    // IMPORTANT: init in AP mode directly — the hosted transport cannot be
    // torn down and re-inited (STA init -> mode(AP) switch fails with
    // "reconfiguring not allowed" + SDIO card init errors). Version check and
    // slave OTA work over the link in any mode.
    Serial.println("[C6] initializing hosted WiFi (SDIO handshake)...");
    WiFi.mode(WIFI_AP);
    delay(500);

    if (!hostedIsInitialized()) {
        Serial.println("[C6] ERROR: hosted init FAILED — SDIO link or slave firmware problem");
        Serial.println("[C6] fallback would be UART flashing via JP1 (see field notes)");
        return;
    }

    hostedGetSlaveVersion(&maj, &min_, &pat);
    Serial.printf("[C6] slave (C6) firmware: %lu.%lu.%lu\n", maj, min_, pat);

    if (hostedHasUpdate()) {
        Serial.println("[C6] version mismatch — updating slave from embedded image");
        if (updateSlaveFromEmbedded()) {
            delay(500);
            ESP.restart();
        }
        return;
    }

    // Versions match — run the SoftAP test exactly like PitServer does it
    Serial.println("[C6] versions match — starting SoftAP test");
    WiFi.softAP(AP_SSID, AP_PASS);
    delay(100);
    IPAddress ip(192, 168, 1, 3);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
    Serial.printf("[C6] SoftAP up: SSID=%s pass=%s ip=%s\n",
                  AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
}

void loop(void) {
    static unsigned long last = 0;
    if (millis() - last >= 5000) {
        Serial.printf("[C6] stations=%d heap=%lu\n",
                      WiFi.softAPgetStationNum(), (unsigned long)ESP.getFreeHeap());
        last = millis();
    }
    delay(100);
}
