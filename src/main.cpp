#include <Arduino.h>
#include <ESP32MQTTClient.h>
#include <WiFi.h>
//------ you should create your own wifi.h with
//#define WLAN_SSID "your_ssid"
//#define WLAN_PASS "your_password"
//#define MQTT_URI "mqtt://x.x.x.x:1883"
//#define MQTT_USERNAME ""
//#define MQTT_PASSWORD ""
#include "wifi.h"
#include "globals.hpp"
#include "trumaframes.hpp"
#include "settings.hpp"
#include <Lin_Interface.hpp>
#include "waterboost.hpp"
#include "commandreader.hpp"
#ifdef WEBSERVER
#include "webserver.hpp"
#endif
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

//adapt these to your board
#ifdef C3
#define RED_LED 3
#define LED 4
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED 2
#define LED_ON HIGH
#define LED_OFF LOW
#endif
#define TX_PIN 19
#define RX_PIN 18

//Lin master
Lin_Interface LinBus(1); 

//Waterboost management
TWaterBoost *WaterBoost;

//the frames to be read
#define FRAMES_TO_READ 5
TFrameBase * frames_to_read[FRAMES_TO_READ];
 //I only care about frame 16 content, the remaining frames will be 
 //directly allocated in frames_to_read
TFrame16 *Frame16;
//the frames to be written
#define FRAMES_TO_WRITE 6
TFrameBase * frames_to_write[FRAMES_TO_WRITE];
TFrameSetTemp *SimulateTempFrame;
TFrameSetTemp *RoomSetpointFrame;
TFrameSetTemp *WaterSetpointFrame;
TFrameEnergySelect *EnergySelect;
TFrameSetPowerLimit *SetPowerLimit;
TFrameSetFan *FanFrame;
//the setpoints
#define SETPOINTS 4
TMqttSetting * MqttSetpoint[SETPOINTS];
TTempSetting *SimulatedTemp;
TTempSetting *RoomSetpoint;
TBoilerSetting *WaterSetpoint;
TFanSetting *FanMode;

//master frames
#define MASTER_FRAMES 5
TMasterFrame * master_frames[MASTER_FRAMES];
//I'll only need to manage these 2 master frames,
//the remaininig one will be directly allocated in master_frames
TOnOff *onOff;
TGetErrorInfo *getErrorInfo;

//only one master frame is sent for each bus cycle, keep track of the next one
int current_master_frame=-1;

//error reset requested
boolean truma_reset=false;
//error reset requested and truma in status 1, stop communication
boolean truma_reset_stop_comm=false;
//new web client or new connection to the broker, force a 
//send of the frame received data
boolean doforcesend=false;
//keep truma on (websocket or screen active)
boolean forceon=false;
//delay to stop the communication during reset
unsigned long truma_reset_delay;
//max time to wait for the truma to report status 1 during reset
unsigned long truma_reset_max_time;
//time to keep the truma on when there's nothing on
//(necessary to switch the fan speed to 0)
unsigned long off_delay;

boolean inota=false;

//serial port commands
TCommandReader CommandReader;

//publish to the mqtt broker/websocket client that an error reset is under way
TPubBool PublishReset("/reset");
//publish to the mqtt broker/websocket client that the lin bus comm is ok
TPubBool PublishLinOk("/linok");
//heartbeat (for the mqtt clients to detect this program is working)
TPubBool PublishHeartBeat("/heartbeat");

//start an error reset
void HandleCommandReset();
//led in a separate task
void LedLoop(void * pvParameters);

#ifdef WEBSERVER
//message from the websocket
void wsCallback(const String& topic, const String& payload);
//new client connected
void wsConnected();
#endif

//wifi status and delay to try and restart it
bool wifistarted=false;
unsigned long lastwifi;

//conditions for the led
bool wifiok=false;
bool trumaok=false;
bool mqttok=false;

//---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
 
  LinBus.baud=9600;
  LinBus.txPin=TX_PIN;
  LinBus.rxPin=RX_PIN;
  LinBus.verboseMode=0;

  pinMode(LED,OUTPUT);
  #ifdef RED_LED
  pinMode(RED_LED,OUTPUT);
  #endif 

  //frames to read
  //frames 0x14 and 0x37 are defined but are useless for this model of combi D
  //so they're not read
  Frame16=new TFrame16();
  frames_to_read[0] = Frame16;
  frames_to_read[1] = new TFrame34();
  frames_to_read[2] = new TFrame39();
  frames_to_read[3] = new TFrame35();
  frames_to_read[4] = new TFrame3b();

  //frames to write
  SimulateTempFrame = new TFrameSetTemp(0x02);
  SimulateTempFrame->setTemperature(-273.0);
  RoomSetpointFrame = new TFrameSetTemp(0x03);
  WaterSetpointFrame = new TFrameSetTemp(0x04);
  EnergySelect = new TFrameEnergySelect(0x05);
  SetPowerLimit = new TFrameSetPowerLimit(0x06);
  FanFrame = new TFrameSetFan(0x07);
  frames_to_write[0] = SimulateTempFrame;
  frames_to_write[1] = RoomSetpointFrame;
  frames_to_write[2] = WaterSetpointFrame;
  frames_to_write[3] = EnergySelect;
  frames_to_write[4] = SetPowerLimit;
  frames_to_write[5] = FanFrame;

  //master frames
  master_frames[0] = new TAssignFrameRanges(0x09, {0x3b, 0x3a, 0x39, 0x38});
  master_frames[1] = new TAssignFrameRanges(0x0d, {0x37, 0x36, 0x35, 0x34});
  master_frames[2] = new TAssignFrameRanges(0x11, {0x33, 0x32, 0xff, 0xff});
  onOff = new TOnOff();
  master_frames[3] = onOff;
  getErrorInfo = new TGetErrorInfo();
  master_frames[4] = getErrorInfo;

  //setpoints
  SimulatedTemp = new TTempSetting("/simultemp", -273.0, 30.0);
  RoomSetpoint = new TTempSetting("/temp", 0.0, 30.0);
  WaterSetpoint = new TBoilerSetting("/boiler");
  FanMode = new TFanSetting("/fan");
  MqttSetpoint[0] = SimulatedTemp;
  MqttSetpoint[1] = RoomSetpoint;
  MqttSetpoint[2] = WaterSetpoint;
  MqttSetpoint[3] = FanMode;

  //waterboost
  WaterBoost = new TWaterBoost(WaterSetpoint,"high","/waterboost");

  //starts the wifi (loop will check if it's connected)
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  lastwifi=millis();

  //starts the mqtt connection to the broker
  mqttClient.setURI(MQTT_URI, MQTT_USERNAME, MQTT_PASSWORD);
  mqttClient.enableLastWillMessage("truma/lwt", "I am going offline");
  mqttClient.setKeepAlive(30);
  mqttClient.enableDebuggingMessages(false);
  mqttClient.loopStart();

  //creates the led task
  xTaskCreate (
      LedLoop,     // Function to implement the task
      "LedLoop",   // Name of the task
      1000,      // Stack size in bytes
      NULL,      // Task input parameter
      1,         // Priority of the task
      NULL      // Task handle
    );
  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else {// U_SPIFFS
          type = "filesystem";
          LittleFS.end();
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        inota=true;
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        inota=false;
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        esp_task_wdt_reset();
      })
      .onError([](ota_error_t error) {
        inota=false;
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
      });
  esp_task_wdt_init(10,true);
  esp_task_wdt_add(NULL);    
}

//enable/disables the master frames to assign frame ranges
void AssignFrameRanges(boolean on) {
  for (int i=0; i<3; i++) {
    master_frames[i]->setEnabled(on && !truma_reset);
  }
}

// When performing a reset only enable the onOff master frame
void EnableOnlyOnOff(boolean on) {
  for (int i=0; i<MASTER_FRAMES; i++) {
    master_frames[i]->setEnabled(!on);
  }
  if (on) {
    onOff->setEnabled(true);
  }
}

// finds the next enabled master frame
void NextMasterFrame() {
  current_master_frame++;
  if (current_master_frame>=MASTER_FRAMES) {
    current_master_frame=0;
  }
  int count=MASTER_FRAMES;
  while (count>0) { 
    if (master_frames[current_master_frame]->getEnabled()) {
      return;
    }
    count--;
    current_master_frame++;
    if (current_master_frame>=MASTER_FRAMES) {
      current_master_frame=0;
    }
  }
  //no master frame enabled
  current_master_frame=-1;
}

//checks and restart the wifi connection
void CheckWifi() {
  // check wifi connectivity
  if (WiFi.status()==WL_CONNECTED) {
    if (!wifiok) {
      ArduinoOTA.setHostname("truminus");
      ArduinoOTA.begin();
    }
    wifiok=true;
    lastwifi=millis();
    if (!wifistarted) {
      Serial.print("IP address ");
      Serial.println(WiFi.localIP());
      wifistarted=true;
      #ifdef WEBSERVER
      //start the web server the first time the wifi is connected
      StartServer(wsCallback, wsConnected);
      #endif
    } 
  } else {
    if (wifiok) {
      ArduinoOTA.end();
      inota=false;
    }
    wifiok=false;
    unsigned long elapsed=millis()-lastwifi;
    if (elapsed>10000) {
    WiFi.reconnect();
    lastwifi=millis();
    }
  }
}

//-------------------------------------------------------------------------
void loop() {
  esp_task_wdt_reset();
  CheckWifi();
  if (wifiok) {
    ArduinoOTA.handle();
  }

  byte PumpOrFan;
  double LocSetPointTemp = 0.0;
  double LocRoomSetpoint = RoomSetpoint->getFloatValue();
  double LocWaterSetpoint = WaterSetpoint->getFloatValue();
  int LocFanMode = FanMode->getIntValue();


  //determines operating mode
  if (LocRoomSetpoint<5.0) {
     //no heating
     if (LocWaterSetpoint<=0.0) {
       //no boiler 
       if (LocFanMode>0) {
        PumpOrFan = 0x10 | LocFanMode;
       } else {
        //setting it to off (0) would turn on water heating,
        //so I just leave it on with speed 0
        PumpOrFan = 0x10; 
       }
     } else {
      //boiler on
      PumpOrFan=0;
     }
  } else {
    //heating active
    LocSetPointTemp = LocRoomSetpoint;
    //force the setting of the fan if it's not already ok for heating
    if (LocFanMode!=-1 && LocFanMode!=-2) {
      if (LocFanMode==2) {
        FanMode->setValue("high",true);
        LocFanMode=-2;
      } else {
        FanMode->setValue("eco",true);
        LocFanMode=-1;
      }
    } 
    if (LocFanMode==-2) {
      PumpOrFan = 2; //high
    } else {
      PumpOrFan = 1; //eco
    }
  } 

  //Water boost
  if (LocWaterSetpoint>=60.0) {
     WaterBoost->Start(Frame16->getWaterDemand());
  } else {
     WaterBoost->Stop();
  }
  //stop heating with water boost and water temperature less than 50ºC
  if (WaterBoost->Active(Frame16->getWaterDemand()) && Frame16->getWaterTemp()<50.0) {
      LocSetPointTemp=0.0;
      PumpOrFan=0;
  }
  
  //prepare setpoint frames
  SimulateTempFrame->setTemperature(SimulatedTemp->getFloatValue());
  RoomSetpointFrame->setTemperature(LocSetPointTemp);
  WaterSetpointFrame->setTemperature(LocWaterSetpoint);
  FanFrame->setPumpOrFan(PumpOrFan);

  //truma reset requested, turn off and wait
  if (truma_reset) {
    onOff->SetOn(false);
    if (!truma_reset_stop_comm ) {
      unsigned long elapsed=millis()-truma_reset_max_time;
      if (elapsed>120000) {
        truma_reset=false;
        Serial.println("Reset time exceeded");
        EnableOnlyOnOff(false);
      }
    }
  } else
  //no reset requested, turn on the heater if is there something active
  if (LocSetPointTemp>=5.0 || LocWaterSetpoint > 0.0 || LocFanMode > 0 || forceon) {
    onOff->SetOn(true);
    forceon = false;
    off_delay = millis();
  } else {
    //keep the heater on for a while (so it can send the frames to stop the fan without activating the boiler)
    if (onOff->GetOn()) {
       unsigned long elapsed=millis()-off_delay;
       if (elapsed>20000) {
        onOff->SetOn(false);
       }
    }
  } 

  if (truma_reset_stop_comm) {
    //stop the communication for 10 seconds to reset the error
    unsigned long elapsed=millis()-truma_reset_delay;
    if (elapsed>10000) {
      truma_reset=false;
      truma_reset_stop_comm=false;
      EnableOnlyOnOff(false);
      Serial.println("Reset done, restarting communication");
    }
  } else {
    //normal cycle
    //force send requested (new websocket client or connection to the broker)
    if (doforcesend) {
      for (int i=0; i<FRAMES_TO_READ; i++) {
        frames_to_read[i]->setForcesend();
      }
      for (int i=0; i<MASTER_FRAMES; i++) {
        master_frames[i]->setForcesend();
      }
      PublishLinOk.setForcesend();
      PublishReset.setForcesend();
      PublishHeartBeat.setForcesend();
      doforcesend=false;
    }
    //read all the frames
    for (int i=0; i<FRAMES_TO_READ; i++) {
      if (LinBus.readFrame(frames_to_read[i]->frameid(),8)) {
        frames_to_read[i]->setReadResult(true);
        frames_to_read[i]->setData((uint8_t*)&(LinBus.LinMessage));
      } else {
        frames_to_read[i]->setReadResult(false);
      }
    }

    //if all the extra frames can be read successfully, there is no need to send
    //the master frames to assign frame ranges
    boolean extraFramesOk=true;
    for (int i=1; i<FRAMES_TO_READ; i++) {
       if (!frames_to_read[i]->getDataOk()) {
          extraFramesOk=false;
          break;
       }
    }
    AssignFrameRanges(!extraFramesOk);

    //for the time being frame 16 is used to check the lin bus communication
    //(perhaps some more frames should be taken into account)
    trumaok=Frame16->getDataOk();
    //write all the frames
    for (int i=0; i<FRAMES_TO_WRITE; i++) {
      frames_to_write[i]->getData((uint8_t*)&(LinBus.LinMessage));
      LinBus.writeFrame(frames_to_write[i]->frameid(),8);
    }
    //and send the next master frame
    NextMasterFrame();
    if (current_master_frame>=0) {
      master_frames[current_master_frame]->getData((uint8_t*)&(LinBus.LinMessage));
      LinBus.writeFrame(0x3c,8);
      if (LinBus.readFrame(0x3d,8)) {
        //setData will call ReadResult if the data is valid
        master_frames[current_master_frame]->setData((uint8_t*)&(LinBus.LinMessage));
        if (truma_reset && master_frames[current_master_frame]==onOff &&  onOff->getCurrentState()==1) {
          truma_reset_delay=millis();
          truma_reset_stop_comm=true;
          Serial.println("truma reset: truma off, stopping communication for 10 seconds");
        }
      } else {
        master_frames[current_master_frame]->setReadResult(false);
      }
    }
  }

  //report the status of the reset and the lin bus communication
  PublishReset.setValue(truma_reset);
  PublishLinOk.setValue(trumaok);
  //and the heartbeat
  PublishHeartBeat.setValue(true);

 /*
  Serial.print(ESP.getFreeHeap());
  Serial.print(", ");
  Serial.println(ESP.getFreePsram());
*/

  //accept a command from the serial port
  String cmd;
  String param;
  boolean found=false; 
  if (CommandReader.Available(&cmd, &param)) {
    if (cmd=="help") {
       Serial.println("Available commands:");
       Serial.println("lindebug on|off");
       Serial.println("mqttdebug on|off");
       Serial.println("reset");
       Serial.println("simultemp -273.0 (off)..30.0");
       Serial.println("temp 0.0 (off)..30.0");
       Serial.println("boiler off|eco|high|boost");
       Serial.println("fan off|eco|high|1..10");   
       found=true;   
    } else if (cmd=="lindebug") {
      if (param=="on") {
        LinBus.verboseMode=1;
        found=true;
      } else if (param=="off") {
        LinBus.verboseMode=0;
        found=true;
      }
    } else if(cmd=="mqttdebug") {
      if (param=="on" || param=="off") {
        mqttClient.enableDebuggingMessages(param=="on");
        found=true;
      }
    } else if(cmd=="reset") {
      found=true;
      HandleCommandReset();
    } else {
      for (int i=0; i<SETPOINTS ;i++) {
        if (MqttSetpoint[i]->MqttMessage(BaseTopicSet+"/"+cmd,param,true)) {
          found=true;
          break;
        }
      }
    }
    if (!found) {
      Serial.println("unkwnown command "+cmd+" "+param);
    }
  }
  #ifdef WEBSERVER
  ws.cleanupClients();
  #endif
}

/* mqtt handling */
esp_err_t handleMQTT(esp_mqtt_event_handle_t event) {
  if (event->event_id==MQTT_EVENT_DISCONNECTED || event->event_id == MQTT_EVENT_ERROR) {
    mqttok=false;
  } 
  mqttClient.onEventCallback(event);
  return ESP_OK;
}

//start an error reset (either from an mqtt or websocket message)
void HandleCommandReset() {
  if (truma_reset) {
    Serial.println("truma reset already active");
  } else {
    truma_reset=true;
    truma_reset_delay=millis();
    EnableOnlyOnOff(true);
    truma_reset_max_time=millis();
    Serial.println("truma reset requested");
  }
}

//checks the mqtt/websocket message to adjust a setpoint
void handleSetting(const String& topic, const String& payload, boolean local)  {
    //manage the error reset here
    if (topic=="truma/set/error_reset" && payload=="1") {
      HandleCommandReset();
      return;
    }   

    //keep the truma on (to refresh values)
    if (topic=="truma/set/ping") {
      forceon = true;
      return;
    }

    //refresh request
    if (topic=="truma/set/refresh") {
      forceon=true;
      doforcesend=true; 
      return;
    }
    //otherwise pass it on to each mqtt setting
    for (int i=0;i<SETPOINTS;i++) {
      if (MqttSetpoint[i]->MqttMessage(topic, payload,local)) 
        break;
    }
}

// message received from the mqtt broker
void callback(const String& topic, const String& payload) {
  mqttok=true;
  Serial.print("Received mqtt message [");
  Serial.print(topic);
  Serial.print("] payload \"");
  Serial.print(payload);
  Serial.println("\"");
  Serial.flush();
  handleSetting(topic,payload,false);
}

#ifdef WEBSERVER
// message received from the websocket (treat is as local)
void wsCallback(const String& topic, const String& payload)  {
  String locTopic=BaseTopicSet+topic;
  handleSetting(locTopic,payload,true);
}
//callback when there is a new websocket client
//(actually called with a special initialization message from the client,
//right after the connection the client isn't yet ready to receive)
void wsConnected() {
  Serial.println("wsConnected callback");
  //sends the current settings
  for (int i=0; i<SETPOINTS ;i++) {
    MqttSetpoint[i]->PublishValue(false);
  }
  //force publish the next received data
  doforcesend=true;
}
#endif

// connection to the broker established, subscribe to the settings and
// force publish the next received data
void onConnectionEstablishedCallback(esp_mqtt_client_handle_t client) {
  doforcesend=true;
  mqttok=true;
  mqttClient.subscribe(BaseTopicSet+"/#", callback);
}

//------------------------------------------------------------------------
void LedLoop(void * pvParameters) {
  int flashes;
  while(1) {
    while (inota) {
      #ifdef RED_LED
      digitalWrite(RED_LED, LED_OFF);
      #endif
      digitalWrite(LED,LED_ON);
      delay(50);
      digitalWrite(LED,LED_OFF);
      delay(50);
    }
    delay(500);
    if (wifiok && trumaok && mqttok && !truma_reset) {
      #ifdef RED_LED
      digitalWrite(RED_LED, LED_OFF);
      #endif
      digitalWrite(LED,LED_ON);
      continue;
    }
    if (!wifiok) {
      flashes=1;
    } else if (!mqttok) {
      flashes=2;
    } else if (!trumaok) {
      flashes=3;
    } else if (truma_reset) {
      flashes=4;
    }
    while (flashes-->0) {
      #ifdef RED_LED
      digitalWrite(LED, LED_OFF);
      digitalWrite(RED_LED,LED_ON);
      delay(200);
      digitalWrite(RED_LED,LED_OFF);
      delay(200);
      #else
      digitalWrite(LED,LED_ON);
      delay(200);
      digitalWrite(LED,LED_OFF);
      delay(200);
      #endif
    }
  }
}
