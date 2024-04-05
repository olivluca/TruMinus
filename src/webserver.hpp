#pragma once
#ifdef WEBSERVER
#include "LittleFS.h"
#include <ArduinoJson.h>
#include "globals.hpp"

typedef std::function<void(const String &topicStr, const String &message)> WebsocketCallback;
typedef std::function<void()> WebsocketConnected;

inline AsyncWebServer server(80);
inline WebsocketCallback wsCb=NULL;
inline WebsocketConnected wsConn=NULL;

void StartServer(WebsocketCallback cb, WebsocketConnected conn);

#endif 