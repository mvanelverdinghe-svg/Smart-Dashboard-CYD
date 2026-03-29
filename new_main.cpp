#include <Arduino.h>
#include <WiFi.h>
#include "webserver.h"

// Vul hier je eigen WiFi-gegevens in
const char* ssid = "JOUW_WIFI_SSID";
const char* password = "JOUW_WIFI_WACHTWOORD";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Opstarten...");
  
  // Verbinden met WiFi
  WiFi.begin(ssid, password);
  Serial.print("Verbinden met WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  Serial.print("Verbonden! IP-adres: ");
  Serial.println(WiFi.localIP());
  
  // Start de webserver
  setupWebServer();
}

void loop() {
  // Laat de webserver inkomende verzoeken afhandelen
  handleClient();
}
