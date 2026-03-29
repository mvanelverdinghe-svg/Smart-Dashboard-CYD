/*******************************************************************************
 * PROJECT: ESP32 CYD (Cheap Yellow Display) - Smart Home Dashboard
 * HARDWARE: ESP32-2432S028R (2.8" ILI9341 TFT + XPT2046 Touch)
 * VERSION: 2.0 (MQTT, OTA, 4-Screen UI)
 * 
 * --- HARDWARE CONFIGURATION ---
 * 1. DISPLAY: ILI9341 (SPI) | Rotation: 1 (Landscape) | Inversion: ON
 * 2. TOUCH: XPT2046 (SPI) | IRQ: Pin 36 | CS: Pin 33
 * 3. LDR (Light Sensor): Pin 39 (Shared with MISO)
 * 4. RGB LED: Red: 4, Green: 17, Blue: 16 (Active Low: LOW=ON, HIGH=OFF)
 *******************************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "include/secrets.h"


// -- PIN DEFINITIES --
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define LED_R 4
#define LED_G 17 
#define LED_B 16 
#define TFT_BL 21
#define LDR_PIN 39 

// --- NETWERKINSTELLINGEN ---
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = SECRET_MQTT;
const char* topic_data = "esp32/cyd/data";
const char* topic_alarm = "esp32/cyd/alarm";

// --- OBJECTEN ---
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();
WiFiClient espClient;
PubSubClient client(espClient);

// --- STATE MANAGEMENT ---
enum Screen { HOME, TRENDS, ACTIVITY, DIALS, ALARM_SCR };
Screen currentScreen = HOME;

float curTemp = 0, curGas = 0, curElek = 0;
int curPower = 0, curSolar = 0;
String curTime = "00:00";
int ldrWaarde = 0;
bool isAlarmActive = false, alarmAcknowledged = false;

int powerHistory[20], solarHistory[20];
struct LogEntry { float t; int p; int s; String time; };
LogEntry activityLog[5];
int historyIdx = 0, logIdx = 0;

unsigned long laatsteTouch = 0, laatsteLDR = 0, laatsteMqttPoging = 0, ldrStartTijd = 0;
bool ldrBezig = false;

void setLedKleur(bool r, bool g, bool b) {
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

void tekenVerbindingsStatus() {
  uint16_t statusKleur = (client.connected()) ? TFT_GREEN : TFT_RED;
  tft.fillCircle(305, 15, 4, statusKleur);
  tft.drawCircle(305, 15, 5, TFT_WHITE);
}

void tekenNavigatie() {
  if (currentScreen == ALARM_SCR) return;
  tft.drawFastHLine(0, 200, 320, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("<", 25, 220);
  tft.drawString(">", 295, 220);
  tft.setTextSize(1);
  const char* namen[] = {"DASHBOARD", "TRENDS", "LOGBOEK", "VERBRUIK DAG"};
  tft.drawString(namen[currentScreen], 160, 220);
  for(int i=0; i<4; i++) {
    tft.fillCircle(130 + (i*20), 235, 2, (currentScreen == i) ? TFT_SKYBLUE : 0x4208);
  }
}

void tekenHome() {
  tft.fillScreen(TFT_BLACK);
  tekenNavigatie();
  tekenVerbindingsStatus();
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE); tft.setTextDatum(TL_DATUM);
  tft.drawString(curTime, 10, 10);
  tft.setTextSize(2); tft.setTextDatum(MC_DATUM);
  tft.drawString("HOME AUTOMATION", 160, 20);
  tft.fillRoundRect(15, 45, 140, 65, 10, 0x2104);
  tft.setTextColor(TFT_ORANGE); tft.setCursor(35, 60); tft.printf("%.1f C", curTemp);
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY); tft.drawString("TEMPERATUUR BUITEN", 85, 100);
  tft.fillRoundRect(165, 45, 140, 65, 10, 0x2104);
  tft.setTextSize(2); tft.setTextColor(TFT_YELLOW); tft.setCursor(185, 60); tft.printf("%d W", curSolar);
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY); tft.drawString("SOLAR POWER", 235, 100);
  tft.fillRoundRect(15, 120, 290, 70, 10, 0x2104);
  tft.setTextColor(TFT_SKYBLUE); tft.setTextSize(3); tft.setCursor(100, 135); tft.printf("%d W", curPower);
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY); tft.drawString("NETTO VERBRUIK", 160, 180);
}

void tekenTrends() {
  tft.fillScreen(TFT_BLACK);
  tekenNavigatie();
  tekenVerbindingsStatus();
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE); tft.setTextDatum(MC_DATUM);
  tft.drawString("POWER TRENDS (W)", 160, 12);
  tft.drawFastHLine(70, 28, 15, TFT_YELLOW); tft.drawString("SOLAR", 105, 28);
  tft.drawFastHLine(180, 28, 15, TFT_SKYBLUE); tft.drawString("NETTO", 215, 28);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString("4k", 20, 45); tft.drawString("2k", 20, 115); tft.drawString("0", 20, 185);
  tft.drawRect(35, 40, 275, 150, TFT_DARKGREY);
  tft.drawFastHLine(35, 115, 275, 0x2104); 
  for(int i=0; i<19; i++) {
    int sy1 = map(solarHistory[i], 0, 4000, 185, 45);
    int sy2 = map(solarHistory[i+1], 0, 4000, 185, 45);
    tft.drawLine(i*14+40, sy1, (i+1)*14+40, sy2, TFT_YELLOW);
    int ny1 = map(powerHistory[i], 0, 4000, 185, 45);
    int ny2 = map(powerHistory[i+1], 0, 4000, 185, 45);
    tft.drawLine(i*14+40, ny1, (i+1)*14+40, ny2, TFT_SKYBLUE);
  }
}

void tekenActivity() {
  tft.fillScreen(TFT_BLACK);
  tekenNavigatie();
  tekenVerbindingsStatus();
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE); tft.setTextDatum(MC_DATUM);
  tft.drawString("RECENT ACTIVITY", 160, 20);
  tft.setTextSize(1); tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString("TIJD", 35, 45); tft.drawString("TEMP", 95, 45); tft.drawString("NETTO", 175, 45); tft.drawString("SOLAR", 265, 45);
  tft.drawFastHLine(20, 52, 280, TFT_DARKGREY);
  for(int i=0; i<5; i++) {
    int idx = (logIdx - 1 - i + 5) % 5;
    if(activityLog[idx].time == "") continue;
    int y = 70 + (i*24);
    tft.setTextColor(TFT_DARKGREY); tft.drawString(activityLog[idx].time, 35, y);
    tft.setTextColor(TFT_ORANGE); tft.setCursor(80, y-4); tft.printf("%.1f C", activityLog[idx].t);
    tft.setTextColor(TFT_SKYBLUE); tft.setCursor(160, y-4); tft.printf("%d W", activityLog[idx].p);
    tft.setTextColor(TFT_YELLOW); tft.setCursor(250, y-4); tft.printf("%d W", activityLog[idx].s);
    tft.drawFastHLine(20, y+10, 280, 0x2104);
  }
}

void tekenDials() {
  tft.fillScreen(TFT_BLACK);
  tekenNavigatie();
  tekenVerbindingsStatus();
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE); tft.setTextDatum(MC_DATUM);
  tft.drawString("DAGVERBRUIK", 160, 25);
  tft.drawCircle(80, 110, 40, TFT_DARKGREY);
  float angle1 = map(constrain(curGas*10, 0, 100), 0, 100, 135, 405) * DEG_TO_RAD;
  tft.drawLine(80, 110, 80 + cos(angle1)*35, 110 + sin(angle1)*35, TFT_ORANGE);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.drawString("GAS (m3)", 80, 160);
  tft.setCursor(65, 105); tft.printf("%.2f", curGas);
  tft.drawCircle(240, 110, 40, TFT_DARKGREY);
  float angle2 = map(constrain(curElek*2, 0, 100), 0, 100, 135, 405) * DEG_TO_RAD;
  tft.drawLine(240, 110, 240 + cos(angle2)*35, 110 + sin(angle2)*35, TFT_GREEN);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1); tft.drawString("ELEK (kWh)", 240, 160);
  tft.setCursor(225, 105); tft.printf("%.1f", curElek);
}

void tekenAlarm() {
  currentScreen = ALARM_SCR;
  tft.fillScreen(TFT_RED);
  setLedKleur(1, 0, 0);
  tft.setTextColor(TFT_WHITE); tft.setTextDatum(MC_DATUM);
  tft.setTextSize(4); tft.drawString("ALARM!", 160, 80);
  tft.setTextSize(2); tft.drawString("VERBRUIK TE HOOG", 160, 130);
  tft.fillRoundRect(60, 170, 200, 50, 10, TFT_WHITE);
  tft.setTextColor(TFT_RED); tft.drawString("ACKNOWLEDGE", 160, 195);
}

void setupOTA() {
  ArduinoOTA.setHostname("CYD-Smart-Dashboard");
  ArduinoOTA.onStart([]() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("OTA UPDATE START", 160, 100);
    tft.drawRect(50, 140, 220, 20, TFT_WHITE);
  });
  ArduinoOTA.onEnd([]() {
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
    tft.drawString("UPDATE SUCCESS!", 160, 120);
    // Geen delay nodig, de herstart volgt direct
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int percentage = (progress / (total / 100));
    tft.fillRect(52, 142, map(percentage, 0, 100, 0, 216), 16, TFT_SKYBLUE);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(String(percentage) + "%", 160, 175);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    tft.fillScreen(TFT_RED);
    tft.drawString("OTA ERROR!", 160, 120);
  });
  ArduinoOTA.begin();
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (String(topic) == topic_data) {
    JsonDocument doc; deserializeJson(doc, payload);
    curTemp = doc["temp"]; curPower = doc["power"]; curSolar = doc["solar"];
    curGas = doc["dgas"]; curElek = doc["delek"];
    curTime = doc["time"].as<String>();
    powerHistory[historyIdx] = curPower;
    solarHistory[historyIdx] = curSolar;
    historyIdx = (historyIdx + 1) % 20;
    activityLog[logIdx] = {curTemp, curPower, curSolar, curTime};
    logIdx = (logIdx + 1) % 5;
    if (currentScreen != ALARM_SCR) {
      if (currentScreen == HOME) tekenHome();
      else if (currentScreen == TRENDS) tekenTrends();
      else if (currentScreen == ACTIVITY) tekenActivity();
      else if (currentScreen == DIALS) tekenDials();
    }
  } 
  else if (String(topic) == topic_alarm) {
    String msg = ""; for (int i = 0; i < length; i++) msg += (char)payload[i];
    if (msg == "ON" && !alarmAcknowledged) tekenAlarm();
    else if (msg == "OFF") { 
      alarmAcknowledged = false; 
      if (currentScreen == ALARM_SCR) {
        currentScreen = HOME; tekenHome(); setLedKleur(0,0,0); 
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  setLedKleur(0, 0, 0); 
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi); ts.setRotation(1);
  tft.init(); tft.setRotation(1); tft.invertDisplay(true);
  WiFi.begin(ssid, password);
  setupOTA();
  client.setServer(mqtt_server, 1883); client.setCallback(callback);
  tekenHome();
}

void loop() {
  ArduinoOTA.handle();

  // Non-blocking WiFi en MQTT beheer
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      unsigned long nu = millis();
      if (nu - laatsteMqttPoging > 5000) { // Probeer elke 5 seconden
        laatsteMqttPoging = nu;
        if (client.connect("ESP32_CYD_Client")) { 
          client.subscribe(topic_data); 
          client.subscribe(topic_alarm); 
        }
      }
    } else {
      client.loop();
    }
  }

  if (digitalRead(XPT2046_IRQ) == LOW && ts.touched()) {
    TS_Point p = ts.getPoint();
    int tx = map(p.x, 260, 3623, 0, 320);
    int ty = map(p.y, 324, 3807, 0, 240);
    if (millis() - laatsteTouch > 400) {
      if (currentScreen == ALARM_SCR) {
        if (ty > 170) { alarmAcknowledged = true; currentScreen = HOME; tekenHome(); setLedKleur(0,0,0); }
      } else if (ty > 200) {
        if (tx < 80) currentScreen = (Screen)((currentScreen + 3) % 4);
        else if (tx > 240) currentScreen = (Screen)((currentScreen + 1) % 4);
        if (currentScreen == HOME) tekenHome();
        else if (currentScreen == TRENDS) tekenTrends();
        else if (currentScreen == ACTIVITY) tekenActivity();
        else if (currentScreen == DIALS) tekenDials();
      }
      laatsteTouch = millis();
    }
  } 
  // Non-blocking LDR uitlezing (Pin 39 is gedeeld met MISO)
  unsigned long nu = millis();
  if (!ldrBezig && nu - laatsteLDR > 1000) {
    pinMode(LDR_PIN, ANALOG);
    ldrStartTijd = nu;
    ldrBezig = true;
  }
  
  if (ldrBezig && nu - ldrStartTijd >= 2) {
    ldrWaarde = analogRead(LDR_PIN);
    pinMode(LDR_PIN, INPUT); // Terugzetten voor SPI/Touch
    ldrBezig = false;
    laatsteLDR = nu;
  }
}