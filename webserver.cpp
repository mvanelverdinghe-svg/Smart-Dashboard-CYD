#include "webserver.h"

// Maak een webserver aan op poort 80
WebServer server(80);

// HTML-pagina met knoppen om GPIO's te schakelen
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 GPIO Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    button { padding: 15px 30px; font-size: 20px; margin: 10px; cursor: pointer; }
  </style>
</head>
<body>
  <h1>ESP32 GPIO Controle</h1>
  <p>GPIO 2 (Ingebouwde LED)</p>
  <button onclick="location.href='/toggle?pin=2'">Schakel GPIO 2</button>
  <p>GPIO 4</p>
  <button onclick="location.href='/toggle?pin=4'">Schakel GPIO 4</button>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleToggle() {
  if (server.hasArg("pin")) {
    String pinString = server.arg("pin");
    int pin = pinString.toInt();
    
    // Zorg ervoor dat de pin als OUTPUT is ingesteld
    pinMode(pin, OUTPUT);
    
    // Lees de huidige status en schakel om
    int currentState = digitalRead(pin);
    digitalWrite(pin, !currentState);
    
    // Stuur de gebruiker terug naar de hoofdpagina
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(400, "text/plain", "Geen pin opgegeven");
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "Pagina niet gevonden");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Webserver gestart");
}

void handleClient() {
  server.handleClient();
}
