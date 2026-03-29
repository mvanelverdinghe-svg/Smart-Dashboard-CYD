#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>

// Externe referentie naar de webserver
extern WebServer server;

// Functies om de webserver in te stellen en af te handelen
void setupWebServer();
void handleClient();

#endif
