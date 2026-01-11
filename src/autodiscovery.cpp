#include "autodiscovery.hpp"
#include "globals.hpp"
#include <ArduinoJson.h>

void TAutoDiscovery::PublishAutoDiscovery()
{
    if (FNext) FNext->PublishAutoDiscovery();

    if (Fcomponent == CKUndefined || Ftopic == "" || Fname == "") {
        return;
    }

    // Device definition
    JsonDocument deviceDoc;
    JsonArray identifiers = deviceDoc["identifiers"].to<JsonArray>();
    identifiers.add(HA_DEVICE_ID);
    deviceDoc["name"] = HA_DEVICE_NAME;
    deviceDoc["model"] = HA_DEVICE_MODEL;
    deviceDoc["manufacturer"] = HA_DEVICE_MANUFACTURER;

    JsonDocument doc;
    String topic;
    String uniq_id;
    switch(Fcomponent) {
        case CKNumber:
        case CKSelect:
        case CKButton:
        case CKSwitch:
            topic=BaseTopicSet+"/"+Ftopic;
            uniq_id="truma_set_"+Ftopic+FSuffix;
            break;
        default:
            topic=BaseTopicStatus+"/"+Ftopic;
            uniq_id="truma_st_"+Ftopic+FSuffix;
    };

    // Common fields
    doc["uniq_id"] = uniq_id;
    doc["name"] = Fname;
    doc["has_entity_name"] = true;
    doc["avty_t"] = STATUS_TOPIC;
    doc["dev"] = deviceDoc;
    // Optional fields
    if (Ficon != "") doc["icon"] = Ficon;
    if (Fdevice_class != "") doc["device_class"] = Fdevice_class;
    if (Funit != "") doc["unit_of_measurement"] = Funit;
    if (Fentity_category !="") doc["entity_category"] = Fentity_category;
    if (Fvalue_template !="") doc["value_template"] = Fvalue_template;
    if (Fsuggested_display_precision >= 0) doc["suggested_display_precision"] = Fsuggested_display_precision;

    // Set topics based on component type
    switch(Fcomponent) {
        case CKSensor:
        case CKBinary_sensor:
            doc["stat_t"] = topic;
            break;
        case CKNumber:
        case CKSelect:
        case CKSwitch:
            doc["cmd_t"] = topic;
            doc["stat_t"] = topic;
            doc["ret"] = true;
            break;
        case CKButton:
            doc["cmd_t"] = topic;
            break;
    }

    // Component-specific fields
    if (Fcomponent == CKSwitch || Fcomponent == CKBinary_sensor) {
        doc["payload_on"] = Fpayload_on;
        doc["payload_off"] = Fpayload_off;
    }

    if (Fcomponent == CKButton) {
        doc["payload_press"] = Fpayload_press;
    }

    if (Fcomponent == CKNumber) {
        if (Fmin > HA_FLOAT_MIN) doc["min"] = Fmin;
        if (Fmax < HA_FLOAT_MAX) doc["max"] = Fmax;
        if (Fstep > 0) doc["step"] = Fstep;
        if (Fmode) doc["mode"] = Fmode;
    }

    if (Fcomponent == CKSelect && Foptions) {
        JsonArray options = doc["options"].to<JsonArray>();
        for (int j = 0; j < Foptions->size(); j++) {
            options.add(Foptions->at(j));
        }
    }

    String discovery_topic = HA_DISCOVERY_TOPIC + ComponentName[Fcomponent] + "/truma/" + uniq_id + "/config";
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(discovery_topic, payload, 0, true);
}
