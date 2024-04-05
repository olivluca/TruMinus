#include "webserver.hpp"
#ifdef WEBSERVER
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    Serial.print("Received websocket message ");
    Serial.println(message);
    if (message=="settings") {
      if (wsConn!=NULL) {
        wsConn();
      }
      return; 
    }
    if (message=="ping"){
      return;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error.code()==DeserializationError::Ok) {
      if (doc.containsKey("id") && doc.containsKey("value")) { 
        String id=doc["id"].as<String>();
        String value=doc["value"].as<String>();
        if (wsCb!=NULL) {
          wsCb(id,value);
        }
      } else {
        Serial.println("missing id or value");
      }
    } else {
      Serial.print("Error decoding json: ");
      Serial.println(error.c_str());
    }
  }  
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void StartServer(WebsocketCallback cb,  WebsocketConnected conn) {
  wsCb=cb;
  wsConn=conn;
  if (LittleFS.begin(false)) {
    Serial.println("Starting webserver");

    initWebSocket();

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/index.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");

    // Start server
    server.begin();
  } else {
    Serial.println("**** cannot start LittleFS");
  }
}

#endif
