#pragma once

// Wi-Fi network created at the firing line (laptop/tablet hotspot).
// Replace these before building.
#define WIFI_SSID       "CHANGE_ME"
#define WIFI_PASSWORD   "CHANGE_ME"

// UDP destination. Broadcast is convenient for first tests; if the hotspot
// blocks broadcast, replace this with the receiver's IPv4 address.
#define UDP_DEST_IP     "255.255.255.255"
#define UDP_DEST_PORT   5005

// RP2040 ADC0 is GPIO26 on Raspberry Pi Pico W.
#define ADC_GPIO                26u
#define ADC_INPUT               0u

// First field-test settings.
#define SAMPLE_RATE_HZ          250000u
#define CAPTURE_SAMPLES         4096u
#define UDP_SAMPLES_PER_PACKET  512u

// Trigger is based on absolute deviation from the measured block mean.
// Tune this after observing real sensor/AFE data.
#define TRIGGER_DELTA_COUNTS    400u

// Avoid repeatedly transmitting the ringing from the same acoustic event.
#define EVENT_HOLDOFF_MS        250u
