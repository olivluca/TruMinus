#include "waterboost.hpp"

void TWaterBoost::ShowRemaining()
{
    ShowState("waterboost active, "+String(fremaining)+" minutes remaining");
}

void TWaterBoost::ShowState(String state)
{
    Serial.println(state);
}

void TWaterBoost::Start(bool CurrentWaterRequest)
{
    if (!factive) {
        fstart=millis();
        foldWaterRequest=CurrentWaterRequest;
        factive=true;
        ffinished=false;
        fremaining=40;
        ShowRemaining();
    }
}

void TWaterBoost::Stop()
{
    if (factive) {
        ShowState("waterboost desactivated");
    }
    factive=false;
    ffinished=false;
    fremaining=0;
}

bool TWaterBoost::Active(bool CurrentWaterRequest)
{
    fpubwaterboost->setValue(fremaining);
    if (!factive || ffinished)
      return false;
    unsigned long elapsed = millis() - fstart;
    if (elapsed>=40*60*1000) {
        ShowState("waterbost active for 40 minutes, stopped");
        ffinished=true;
        fremaining=0;
        fmqttsetting->setValue(fstoppayload);
        return false;
    }  
    if (!CurrentWaterRequest && foldWaterRequest) {
        ShowState("waterboost temperature reached, stopped");
        ffinished=true;
        fremaining=0;
        fmqttsetting->setValue(fstoppayload);
        return false;
    }
    foldWaterRequest=CurrentWaterRequest;
    int remaining = 40 - (elapsed / 60000);
    if (remaining != fremaining) {
        fremaining = remaining;
        ShowRemaining();
    }
   return true;
}
