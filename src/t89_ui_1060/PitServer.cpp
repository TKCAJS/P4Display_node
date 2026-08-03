#include "PitServer.h"

#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>
#include <SD_MMC.h>

#include "SDLogger.h"
#include "viewer_html.h"

static WebServer server(80);
static volatile bool toggleRequested = false;
static volatile bool active = false;
static bool routesRegistered = false;

static void handleRoot() {
    server.send_P(200, "text/html", VIEWER_HTML);
}

static void handleLogList() {
    String json = "{\"sd\":";
    json += sdLoggerIsAvailable() ? "true" : "false";
    json += ",\"files\":[";
    File dir = SD_MMC.open("/logs");
    if (dir && dir.isDirectory()) {
        File f;
        bool first = true;
        while ((f = dir.openNextFile())) {
            if (!f.isDirectory()) {
                if (!first) json += ",";
                first = false;
                String name = f.name();
                int slash = name.lastIndexOf('/');
                if (slash >= 0) name = name.substring(slash + 1);
                json += "{\"name\":\"" + name + "\",\"size\":" + String(f.size()) + "}";
            }
            f.close();
        }
        dir.close();
    }
    json += "]}";
    server.send(200, "application/json", json);
}

static void handleDownload() {
    String name = server.arg("f");
    if (name.length() == 0 || name.indexOf('/') >= 0 || name.indexOf("..") >= 0) {
        server.send(400, "text/plain", "bad filename");
        return;
    }
    File f = SD_MMC.open("/logs/" + name);
    if (!f || f.isDirectory()) {
        if (f) f.close();
        server.send(404, "text/plain", "not found");
        return;
    }
    server.sendHeader("Content-Disposition", "attachment; filename=" + name);
    server.streamFile(f, "text/csv");
    f.close();
}

static bool wifiEverStarted = false;

static void start() {
    sdLoggerPause();
    sdLoggerRemount();   // give a card that failed at boot another chance

    // P4/ESP-Hosted: the SDIO transport to the C6 can only be initialized
    // ONCE — WiFi.mode(WIFI_OFF) after use, then mode(WIFI_AP) again, hits
    // "reconfiguring not allowed" + SDIO card init failures. So the first
    // toggle inits the stack normally (Arduino path -> hosted init), and
    // stop()/re-start() only cycle the radio with esp_wifi_stop()/_start()
    // underneath Arduino, which never touches the transport.
    if (!wifiEverStarted) {
        WiFi.mode(WIFI_AP);
        wifiEverStarted = true;
    } else {
        esp_wifi_start();
    }
    WiFi.softAP(PIT_AP_SSID, PIT_AP_PASS);
    delay(100);   // AP must be up before softAPConfig sticks
    IPAddress ip(PIT_AP_IP);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));

    // Every TX burst can sag the 3.3 V rail enough to show as visible display
    // noise, so make bursts rare and small: beacon 4x less often than the
    // 100 TU default, and minimum power that still covers a pit garage.
    wifi_config_t cfg;
    if (esp_wifi_get_config(WIFI_IF_AP, &cfg) == ESP_OK) {
        cfg.ap.beacon_interval = 400;
        esp_wifi_set_config(WIFI_IF_AP, &cfg);
    }
    WiFi.setTxPower(WIFI_POWER_5dBm);

    if (!routesRegistered) {
        server.on("/", handleRoot);
        server.on("/api/logs", handleLogList);
        server.on("/download", handleDownload);
        server.onNotFound([]() { server.send(404, "text/plain", "not found"); });
        routesRegistered = true;
    }
    server.begin();
    active = true;
    Serial.printf("Pit mode ON: SSID=%s pass=%s http://%s/ (free heap %u)\n",
                  PIT_AP_SSID, PIT_AP_PASS, WiFi.softAPIP().toString().c_str(),
                  ESP.getFreeHeap());
}

static void stop() {
    server.stop();
    // Radio off WITHOUT tearing down Arduino/hosted state (see start()):
    // clear the AP config (kicks stations) but keep mode, then stop the WiFi
    // task. esp_wifi_start() in the next start() brings it straight back.
    WiFi.softAPdisconnect(false);
    esp_wifi_stop();
    active = false;
    sdLoggerResume();
    Serial.println("Pit mode OFF: logging resumed");
}

void pitServerRequestToggle() {
    toggleRequested = true;
}

void pitServerService() {
    if (toggleRequested) {
        toggleRequested = false;
        active ? stop() : start();
    }
    if (active) server.handleClient();
}

bool pitServerIsActive() {
    return active;
}
