#include "settings.hpp"
#include <type_traits>
#ifdef WEBSERVER
#include <ArduinoJson.h>
#endif

String BoilerMode[] = {"off","eco","high","boost"};
double BoilerTemp[] = {0.0, 40.0, 55.0, 60.0}; 

void TMqttSetting::PublishValue(bool local)
{
    String value;
    switch (fkind) {
        case SKFloat:
           value=String(ffloatvalue,1);
           break;
        case SKInt:
           value=String(fintvalue);
           break;
        case SKString:
           value=fstringvalue;
           break;   
    }
    if (local) {
      mqttClient.publish(BaseTopicSet+ftopic, value,0,true);
    }
    #ifdef WEBSERVER
    sendToWebsocket(value);
    #endif
}

#ifdef WEBSERVER
void TMqttSetting::sendToWebsocket(const String &value)
{
    JsonDocument doc;
    doc["command"]="setting";
    doc["id"]=ftopic;
    doc["value"]=value;
    ws.textAll(doc.as<String>());
}
#endif

bool TMqttSetting::Validate(int newvalue)
{
    Serial.print("Accepted new int value ");
    Serial.print(newvalue);
    Serial.print(" for ");
    Serial.println(ftopic);
    return true;
}

bool TMqttSetting::Validate(double newvalue)
{
    Serial.print("Accepted new float value ");
    Serial.print(newvalue);
    Serial.print(" for ");
    Serial.println(ftopic);
    return true;
}

bool TMqttSetting::Validate(String newvalue)
{
    Serial.print("Accepted new string value ");
    Serial.print(newvalue);
    Serial.print(" for ");
    Serial.println(ftopic);
    return true;
}

TMqttSetting::TMqttSetting(String topic, SettingKind kind)
{
    ftopic=topic;
    fkind=kind;
}

bool TMqttSetting::MqttMessage(String topic, String payload, boolean local)
{
    if (topic==BaseTopicSet+ftopic) {
        String value;
        switch (fkind) {
            case SKFloat:
              setValue(atof(payload.c_str()),local);
              break;
            case SKInt:
              setValue(atoi(payload.c_str()),local);
              break;
            case SKString:
              setValue(payload,local);
              break;    
        }
        return true;
    }
    return false;
}

void TMqttSetting::setValue(int newvalue, bool local)
{
    if (!Validate(newvalue)||fintvalue==newvalue) {
        return;
    }
    fintvalue=newvalue;
    PublishValue(local);
}

void TMqttSetting::setValue(double newvalue, bool local)
{
    if (!Validate(newvalue)||ffloatvalue==newvalue) {
        return;
    }
    ffloatvalue=newvalue;
    PublishValue(local);
}

void TMqttSetting::setValue(String newvalue, bool local)
{
    if (!Validate(newvalue)||fstringvalue==newvalue) {
        return;
    }
    fstringvalue=newvalue;
    PublishValue(local);
}

bool TBoilerSetting::Validate(String newvalue)
{
    for (int i=0; i<std::extent<decltype(BoilerMode)>::value; i++ ) {
        if (newvalue==BoilerMode[i]) {
            setValue(BoilerTemp[i],false);
            return TMqttSetting::Validate(newvalue);
        }
    }
    Serial.print(newvalue);
    Serial.print(" is not a valid value for ");
    Serial.println(ftopic);
    return false;
}

bool TFanSetting::Validate(String newvalue)
{
    int num=atoi(newvalue.c_str());
    if (String(num)==newvalue && num>=0  && num<=10 ) {
        setValue(num,false);
        return TMqttSetting::Validate(newvalue);
    }
    if (newvalue=="off") {
        setValue(0,false);
        return TMqttSetting::Validate(newvalue);
    }
    if (newvalue=="eco") {
        setValue(-1,false);
        return TMqttSetting::Validate(newvalue);
    }
    if (newvalue=="high") {
        setValue(-2,false);
        return TMqttSetting::Validate(newvalue);
    }
    Serial.print(newvalue);
    Serial.print(" is not a valid value for ");
    Serial.println(ftopic);
    return false;
}

bool TTempSetting::Validate(double newvalue)
{
    if (newvalue>=fminvalue && newvalue<=fmaxvalue) {
        return TMqttSetting::Validate(newvalue);
    }
    Serial.print("temperature ");
    Serial.print(newvalue);
    Serial.print(" out of range ");
    Serial.print(fminvalue);
    Serial.print(" - ");
    Serial.print(fmaxvalue);
    Serial.print(" for ");
    Serial.print(ftopic);
    return false;
}
