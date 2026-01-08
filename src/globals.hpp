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

//used both for the last will message and for the mqtt autodiscovery
#define STATUS_TOPIC "truma/status/online"
#define STATUS_OFFLINE "offline"
#define STATUS_ONLINE "online"

//mqtt autodiscovery parameters
#define HA_DEVICE_ID "truma_boiler_01"
#define HA_DEVICE_NAME "Combi D"
#define HA_DEVICE_MODEL "Combi Heater"
#define HA_DEVICE_MANUFACTURER "Truma"
#define HA_DISCOVERY_TOPIC "homeassistant/"

//default values (no min/max will be set in the autodiscovery payload)
#define HA_FLOAT_MIN -100000.0
#define HA_FLOAT_MAX  100000.0
