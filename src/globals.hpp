#pragma once
#include <Arduino.h>
#include <ESP32MQTTClient.h>
#ifdef WEBSERVER
#include "ESPAsyncWebServer.h"
#endif

inline ESP32MQTTClient  mqttClient;
inline String const BaseTopicStatus = "truma/status";
inline String const BaseTopicSet = "truma/set";
#ifdef WEBSERVER
inline AsyncWebSocket ws("/ws");
#endif