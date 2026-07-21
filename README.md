# 🏠 ESP32 CYD Smart Home Dashboard

A professional, multi-screen monitoring dashboard for the **ESP32-2432S028R** (Cheap Yellow Display). This project integrates with **Node-RED** via **MQTT** to display real-time energy, solar, and environmental data.

Current firmware version: **2.3**.

![Dashboard](images/Start_20260328.jpg)
![Dashboard](images/Table_20260328.jpg)
![Dashboard](images/Oscil_20260328.jpg)
![Dashboard](images/gauges_20260328.jpg)

---

## 🚀 Features

- **4 Interactive Screens**:
  - **Dashboard**: Real-time Outdoor Temperature, Solar Power, and Net Consumption.
  - **Trends**: Dual-line graph showing Solar vs. Net Power history.
  - **Logbook**: Recent activity table with timestamps.
  - **Meters**: Visual gauges for daily Gas (m³) and Electricity (kWh) usage.
- **Smart Connectivity**:
  - Shared **MVWiFi** and **MqttManager** integration for data and alarms.
  - Retained online/offline status and periodic retained health via **MVHealth**.
  - Central **MVOTA** integration with hostname `CYD-Smart-Dashboard` (no OTA password configured).
  - USB uploads through `esptool` and OTA uploads through `espota`.
  - Exclusive **MVOTA display mode** with progress, success, and error feedback.
  - Connection status indicator (Green/Red LED on screen).
- **Hardware Optimizations**:
  - **LDR/MISO Fix**: IRQ-guarded hybrid switching on GPIO39, followed by explicit touch-SPI recovery.
  - Non-blocking periodic LDR measurements that remain mutually exclusive with touch handling.
  - **RGB LED Alarm**: Visual red alert when power consumption exceeds thresholds.
  - **Calibrated Touch**: Precise navigation using the XPT2046 controller.

---

## 🛠 Hardware Requirements

- **Board**: ESP32-2432S028R (2.8" ILI9341 TFT + XPT2046 Touch).
- **Sensors**: Built-in LDR (R21) and RGB LED.
- **Connectivity**: 2.4GHz WiFi.

---

## 📦 Libraries Used

- `TFT_eSPI` (Display driver)
- `XPT2046_Touchscreen` (Touch driver)
- `MV_ESP` (`MVWiFi`, `MqttManager`, `MVHealth`, `MVOTA`)
- `ArduinoJson` (JSON parsing)

---


## ⚙️ Setup and configuration

### WiFi and MQTT

Configure the credentials used by `main.cpp` in `include/secrets.h`:

```cpp
#define SECRET_SSID "YOUR_WIFI_SSID"
#define SECRET_PASS "YOUR_WIFI_PASSWORD"
#define SECRET_MQTT "YOUR_MQTT_SERVER"
```

### Node-RED integration

The dashboard expects a JSON payload on the `esp32/cyd/data` topic:

```json
{
  "temp": 12.5,
  "power": 1500,
  "solar": 2000,
  "dgas": 1.25,
  "delek": 8.4,
  "time": "14:30"
}
```

Alarm commands use the existing `esp32/cyd/alarm` topic.

Connectivity status is retained on `esp32/cyd/status`. Health is retained on
`esp32/cyd/health`, is refreshed every five minutes, and can be requested by publishing
`PING` to `esp32/cyd/health/cmd`.

## 🔌 Upload environments

- `cyd`: USB upload using `esptool`.
- `cyd_ota`: network upload using `espota` to the configured `upload_port`.

Build either environment without uploading:

```bash
pio run -e cyd
pio run -e cyd_ota
```

Upload over USB:

```bash
pio run -e cyd -t upload
```

Upload over WiFi:

```bash
pio run -e cyd_ota -t upload
```

During a real OTA upload the TFT shows `OTA UPDATE`, a progress bar and percentage. MQTT screen
updates, touch navigation, and LDR measurements are temporarily suspended. At completion it shows
`Upload voltooid`; OTA errors display their error code before the normal screen is restored.

The firmware registers its existing start, end, progress, and error callbacks through the shared
`MVOTA` library. MVOTA is initialized only when `MVWiFi` reports an active connection; startup does
not wait in a blocking loop.

## ♻️ Possible future reuse

The display drawing, pages, layout and touch handling remain project-specific CYD code. Reusable
display components should only move to a separate shared library when a second project needs the
same components and their common interface can be demonstrated without changing CYD behavior.

## 📝 License

This project is for personal home automation use. Feel free to modify 
and expand it. Thanks to Paul
Stoffregen and others whose published information about the CYD hardware helped resolve the shared
GPIO39 LDR/touch-MISO behavior.
