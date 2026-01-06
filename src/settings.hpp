#pragma once
#include <ESP32MQTTClient.h>
#include "globals.hpp"

/*
 Setpoint management
 Each setpoint can be either set locally from a websocket client or the serial port
 (in that case the value will be sent to the mqtt broker) or from the mqtt
 broker (and it will be sent to the websocket clients).
*/

enum SettingKind { SKFloat, SKInt, SKString };
class TMqttSetting {
  private:
    SettingKind fkind;
    #ifdef WEBSERVER
    void sendToWebsocket(const String &value);
    #endif 
  protected:
    String ftopic;
    String fstringvalue;
    double ffloatvalue;
    int fintvalue;
    virtual bool Validate(int newvalue);
    virtual bool Validate(double newvalue);
    virtual bool Validate(String newvalue);
  public:
    TMqttSetting(String topic, SettingKind kind);
    //sends the setting to the websocket clients (always) and to the mqtt broker (only if it's local)
    void PublishValue(bool local);
    //checks an incoming mqtt (or websocket client) message
    bool MqttMessage(String topic, String payload, bool local=false);
    //validates, sets and eventually publishes the value
    void setValue(int newvalue, bool local=true);
    void setValue(double newvalue, bool local=true);
    void setValue(String newvalue, bool local=true);
    String getStringValue() { return fstringvalue; };
    int32_t getIntValue()  { return fintvalue; };
    double  getFloatValue() { return ffloatvalue; };
};

class TBoilerSetting : public TMqttSetting {
    protected:
     virtual bool Validate(String newvalue) override;
    public:
      TBoilerSetting(String topic) : TMqttSetting(topic, SKString) {
        setValue("off");
      };
}; 

class TTempSetting : public TMqttSetting {
    private:
      double fminvalue;
      double fmaxvalue;
    protected:
     virtual bool Validate(double newvalue) override; 
    public:
      TTempSetting(String topic, double minvalue, double maxvalue) : TMqttSetting(topic, SKFloat) {
        fminvalue=minvalue;
        fmaxvalue=maxvalue;
        setValue(fminvalue);
      };
}; 

class TFanSetting : public TMqttSetting {
    protected:
     virtual bool Validate(String newvalue) override;
    public:
      TFanSetting(String topic) : TMqttSetting(topic, SKString){
        setValue("off");
      };
}; 

class TOnOffSetting : public TMqttSetting {
    protected:
     virtual bool Validate(String newvalue) override;
    public:
      TOnOffSetting(String topic) : TMqttSetting(topic, SKString){
        setValue(0);
      };
}; 
