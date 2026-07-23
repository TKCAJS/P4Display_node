#ifndef PIT_SERVER_H
#define PIT_SERVER_H

// Pit-mode log server: SoftAP + HTTP server that serves the client-side
// viewer page and the CSV files from the SD card. Off during normal running
// (WiFi contends with the RGB panel for RAM bandwidth); toggled by a 3 s
// long-press on the screen. SD logging is paused while active so downloads
// never race the logger on the SPI bus.

#define PIT_AP_SSID "T89-Pits"
#define PIT_AP_PASS "t89racing"
#define PIT_AP_IP_STR "192.168.1.3"
#define PIT_AP_IP 192, 168, 1, 3

// Request a toggle (safe from any task; applied by pitServerService)
void pitServerRequestToggle();

// Apply pending toggles and handle HTTP clients — call from the protocol task
void pitServerService();

bool pitServerIsActive();

#endif // PIT_SERVER_H
