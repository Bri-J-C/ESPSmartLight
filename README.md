# ESPSmartLight

ESP32-C3 smart light switch with Home Assistant integration via MQTT.

Built for the ESP32-C3 Super Mini paired with a SainSmart 2-channel relay module (active LOW). Provides a web-based configuration portal, MQTT control with Home Assistant auto-discovery, OTA updates, and physical button control.

## Features

- **Home Assistant Integration** — MQTT with auto-discovery as a `light` entity
- **Web Portal** — Configure WiFi, MQTT, device name, GPIO pins, and boot state from a mobile-friendly UI
- **Physical Button** — Short press toggles relay, 3-5s hold clears WiFi config, 10s hold factory resets
- **OTA Updates** — Flash new firmware over WiFi via ArduinoOTA
- **Status LED** — Visual feedback for AP mode, connecting, MQTT status, and relay state
- **Persistent Config** — All settings stored in NVS flash with wear-leveling for relay state
- **WiFi Self-Healing** — Automatic reconnect with exponential backoff; reboots on prolonged failure

## Hardware

| Component | Detail |
|-----------|--------|
| MCU | ESP32-C3 Super Mini |
| Relay | SainSmart 2-channel (active LOW, channel 1) |
| LED | GPIO 8 (onboard, active LOW) |
| Relay Pin | GPIO 3 (configurable) |
| Button Pin | GPIO 9 (onboard BOOT button, configurable) |

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE)
- ESP32-C3 Super Mini connected via USB

### Build & Flash

```bash
pio run                                        # build
pio run -t upload --upload-port /dev/ttyACM0   # flash
```

### First Boot

1. The device starts in AP mode with SSID `SmartLight-XXXX`
2. Connect to the AP and navigate to `192.168.4.1`
3. Enter your WiFi credentials, MQTT broker details, and device name
4. Save — the device reboots and connects to your network

### Home Assistant

The device automatically publishes an MQTT discovery config. After connecting to your MQTT broker, the light entity appears in Home Assistant with no manual configuration needed.

MQTT topics follow the pattern:
```
{mqtt_root}/{hostname}/state    # ON/OFF state (retained)
{mqtt_root}/{hostname}/set      # Command topic
{mqtt_root}/{hostname}/available # Online/offline (retained)
```

## Architecture

9 modules with a global singleton pattern:

| Module | Responsibility |
|--------|---------------|
| `config_manager` | NVS persistence for all settings |
| `wifi_manager` | STA/AP mode, reconnect with backoff, mDNS |
| `mqtt_manager` | MQTT connection, HA discovery, state pub/sub |
| `web_portal` | HTTP server, config UI, status page, log viewer |
| `relay` | GPIO control with NVS write debouncing (60s) |
| `button` | Debounced input with short press, WiFi clear, factory reset |
| `status_led` | Non-blocking LED patterns (blink, double, triple, SOS, etc.) |
| `ota` | ArduinoOTA wrapper |
| `logger` | Ring buffer (50 entries), serial output, color-coded web UI at `/logs` |

## Web Interface

- `/` — Status page (when connected) or config page (in AP mode)
- `/config` — WiFi, MQTT, and device configuration
- `/logs` — Live device logs with 5s auto-refresh
- `/status` — JSON status endpoint

## Button Actions

| Action | Result |
|--------|--------|
| Short press | Toggle relay |
| Hold 3-5s, release | Clear WiFi config and reboot |
| Hold 10s | Factory reset and reboot |

## License

MIT
