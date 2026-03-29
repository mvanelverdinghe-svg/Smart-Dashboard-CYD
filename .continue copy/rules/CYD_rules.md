# ESP32 MQTT Project Regels

## Hardware Setup
 * PROJECT: ESP32 CYD (Cheap Yellow Display) - Smart Home Dashboard
 * HARDWARE: ESP32-2432S028R (2.8" ILI9341 TFT + XPT2046 Touch)
 * VERSION: 2.0 (MQTT, OTA, 4-Screen UI)
 * 
 * --- HARDWARE CONFIGURATION ---
 * 1. DISPLAY: ILI9341 (SPI) | Rotation: 1 (Landscape) | Inversion: ON
 * 2. TOUCH: XPT2046 (SPI) | IRQ: Pin 36 | CS: Pin 33
 * 3. LDR (Light Sensor): Pin 39 (Shared with MISO)
 * 4. RGB LED: Red: 4, Green: 17, Blue: 16 (Active Low: LOW=ON, HIGH=OFF)

## Bibliotheken
- Gebruik altijd `PubSubClient` voor MQTT.
- Gebruik `WiFiClientSecure` niet (we blijven op poort 1883).

## Structuur
- WiFi-gegevens staan in `include/secrets.h`.
- Gebruik geen blokkerende `delay()`, maar `millis()` voor timing.

# Veiligheidsregels voor de Agent
- Je mag NOOIT bestanden aanpassen of lezen buiten de huidige projectmap: ${cwd}.
- Voordat je een bestand aanpast, vraag je altijd expliciet toestemming aan de gebruiker.
- Gebruik geen commando's die systeeminstellingen van Ubuntu wijzigen.