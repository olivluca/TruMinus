#pragma once
#include <Arduino.h>
#include <ESP32MQTTClient.h>
#include "globals.hpp"
#include "settings.hpp"
#include "trumaframes.hpp"

/*
  Waterboost management

  waterboost is not managed by the boiler but by the cp-plus.
  When a waterboost is requested, the water setpoint will be 60ºC
  and it will be kept active for 40 minutes or until the water 
  request is deactivated (meaning, it has reched the setpoint).
  This helper class:
   - keeps track of the time and the status of the water request.
   - will change the setpoint from "boost" to "high" when the 
     waterboost is done (using the mqttsetting parameter)
   - will publish the remaining time to the mqtt broker and
     websocket clients (with the topic parameter)  

*/
class TWaterBoost {
  private:  
    bool factive;
    bool ffinished;
    int fremaining;
    TMqttSetting *fmqttsetting;
    String fstoppayload;
    TMqttPublisherBase *fpubwaterboost;
    unsigned long fstart;
    bool foldWaterRequest;
    void ShowRemaining();
    void ShowState(String state);
  public:  
    TWaterBoost(TMqttSetting *mqttsetting, String stoppayload, String topic) {
       factive = false;
        ffinished = false;
        fmqttsetting=mqttsetting;
        fstoppayload=stoppayload;
        fpubwaterboost = new TMqttPublisherBase(topic);
        };
    void Start(bool CurrentWaterRequest);
    void Stop();
    bool Active(bool CurrentWaterRequest);
};