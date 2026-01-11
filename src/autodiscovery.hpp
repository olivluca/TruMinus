#pragma once
#include <Arduino.h>
#include "globals.hpp"

/**
 * Class to publishe MQTT Home Assistant Auto-Discovery configuration.
 */

enum ComponentKind { CKUndefined, CKNumber, CKSwitch, CKSelect, CKButton, CKSensor, CKBinary_sensor };
const String ComponentName[] = {"","number","switch","select","button","sensor","binary_sensor"};
class TAutoDiscovery {
  private:
    ComponentKind Fcomponent = CKUndefined;
    String Ftopic = "";
    String Fname = "";
    String Ficon = "";
    String Fdevice_class = "";
    String Funit = "";
    String Fentity_category = "";

    // Control-specific
    String Fpayload_on = "1";
    String Fpayload_off = "0";
    String Fpayload_press = "1";

    // Number-specific
    float Fmin=HA_FLOAT_MIN;
    float Fmax=HA_FLOAT_MAX;
    float Fstep=0.1;
    String Fmode;

    // Select-specific
    std::vector<String>* Foptions = nullptr;

    // Template
    String Fvalue_template = "";

    // Other flags
    int Fsuggested_display_precision = -1;

    // Next in chain
    TAutoDiscovery * FNext = nullptr;
    String FSuffix = "";
  public:
    TAutoDiscovery() {};
    TAutoDiscovery* setADComponent(ComponentKind value) {Fcomponent=value; return this;};
    TAutoDiscovery* setADName(String value) {Fname=value; return this;};
    //the topics start with "/", remove it
    TAutoDiscovery* setADTopic(String value) {Ftopic=value;Ftopic.remove(0,1); return this;};
    TAutoDiscovery* setADIcon(String value) {Ficon=value; return this;};
    TAutoDiscovery* setADDevice_class(String value) {Fdevice_class=value; return this;};
    TAutoDiscovery* setADUnit(String value) {Funit=value; return this;};
    TAutoDiscovery* setADEntity_category(String value) {Fentity_category=value; return this;};
    TAutoDiscovery* setADPayload_on(String value) {Fpayload_on=value; return this;};
    TAutoDiscovery* setADPayload_off(String value) {Fpayload_off=value; return this;};
    TAutoDiscovery* setADPayload_press(String value) {Fpayload_press=value; return this;};
    TAutoDiscovery* setADMin(float value) {Fmin=value; return this;};
    TAutoDiscovery* setADMax(float value) {Fmax=value; return this;};
    TAutoDiscovery* setADStep(float value) {Fstep=value; return this;};
    TAutoDiscovery* setADMode(String value) {Fmode=value; return this;};
    TAutoDiscovery* setADSuggested_display_precision(int value) {Fsuggested_display_precision=value; return this;};
    TAutoDiscovery* setADOptions(std::vector<String>* value) {Foptions = value; return this;};
    TAutoDiscovery* setADValue_template(String value) {Fvalue_template=value; return this;};
    TAutoDiscovery* addAutoDiscovery(String suffix) {FNext = new TAutoDiscovery() ; FNext->Ftopic = Ftopic; FNext->FSuffix=suffix; return FNext;};
    void PublishAutoDiscovery();
};
