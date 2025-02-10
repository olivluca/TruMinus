#include "trumaframes.hpp"
#include "globals.hpp"
#include <string.h>
#ifdef WEBSERVER
#include <ArduinoJson.h>
#endif
double RawKelvinToTemp(const uint16_t RawValue) {
  return RawValue / 10.0 - 273.0;
}

void TempToRawKelvin(double temp, uint8_t *dest)
{
    uint16_t rawvalue=htole16(uint16_t((temp+273.0)*10));
    memcpy(dest, &rawvalue, 2);
}
double RawToVoltage(const uint16_t RawValue)
{
    return RawValue/ 100.0 -327.67;
}

double RawToFlameTemperature(const uint8_t RawValue)
{
    double v=RawValue; 
    return pow(v,3.0)* 1.8602209820528515E-05 + pow(v, 2.0) * -0.0004895309102721512 + v * 1.4470709562301636 -65.64685821533203;
};

TMqttPublisherBase::TMqttPublisherBase(String topic)
{
    ftopic=topic;
}

void TMqttPublisherBase::setValue(uint32_t newvalue)
{
    unsigned long now = millis();
    unsigned long elapsed = now-flastsent;
    boolean valuechanged = newvalue!=fvalue;
    fvalue=newvalue;
    String payload=getPayload();
    #ifdef WEBSERVER
    /* the asyncwebsocket gets bogged down if it has to send too many messages
       just send when the value has changed or a new client just connected */
    if (valuechanged || fforcesend) {
        JsonDocument doc;
        doc["command"]="status";
        doc["id"]=ftopic;
        doc["value"]=payload;
        ws.textAll(doc.as<String>());
    }  
    #endif
    if (valuechanged || fforcesend || elapsed>10000) {
        flastsent=now;
        //Serial.print(ftopic);
        //Serial.print(" ");
        //Serial.println(fvalue);
        mqttClient.publish(BaseTopicStatus+ftopic,payload);
    }
    fforcesend=false;
}
void TMqttPublisherBase::setForcesend(){
    fforcesend=true;
};

TFrame14::TFrame14()  : TFrameBase()
{
    fframeid=0x14;
    fpublishers.push_back(new TPubTemperature("/f14_roomtemp"));
    fpublishers.push_back(new TPubTemperature("/f14_roomtargettemp"));
    fpublishers.push_back(new TPubTemperature("/f14_watertargettemp"));
}

void TFrame14::publishFrameData()
{
   frame14Data *locdata=(frame14Data *)&fdata;
   fpublishers[0]->setValue(le16toh(locdata->RoomTemperature));
   fpublishers[1]->setValue(le16toh(locdata->RoomTargetTemperature));
   fpublishers[2]->setValue(le16toh(locdata->WaterTargetTemperature));
}


TFrameBase::TFrameBase()
{
    fdataok = false;
}

void TFrameBase::setReadResult(boolean ok)
{
    if (fdataok!=ok) {
        fdataok=ok;
        Serial.print("read frame ");
        Serial.print(fframeid,16);
        if (ok) {
            Serial.println(" ok");
        } else {
            Serial.println(" error");
        }
    }
}

void TFrameBase::setData(uint8_t *data)
{
    memcpy(fdata, data,8);
    publishFrameData();
}

void TFrameBase::setForcesend()
{
    for (int i=0; i<fpublishers.size(); i++) {
        fpublishers[i]->setForcesend();
    }
}

void TFrameBase::getData(uint8_t *dest)
{
    memcpy(dest, &fdata, 8);
}

void TFrameBase::publishFrameData()
{
    //do nothing in base class
}

TFrame16::TFrame16():TFrameBase()
{
    fframeid=0x16;
    fpublishers.push_back(new TPubBool("/antifreeze"));
    fpublishers.push_back(new TPubBool("/supply220"));
    fpublishers.push_back(new TPubBool("/window"));
    fpublishers.push_back(new TPubBool("/roomdemand"));
    fpublishers.push_back(new TPubBool("/waterdemand"));
    fpublishers.push_back(new TPubBool("/error")); 
    fpublishers.push_back(new TPubTemperature("/room_temp"));
    fpublishers.push_back(new TPubTemperature("/water_temp"));
    fpublishers.push_back(new TPubVoltage("/voltage"));
}

void TFrame16::publishFrameData()
{
    frame16Data *locdata=(frame16Data *) &fdata;
    fpublishers[0]->setValue(locdata->Antifreeze);
    fpublishers[1]->setValue(locdata->Supply220);
    fpublishers[2]->setValue(locdata->Window);
    fpublishers[3]->setValue(locdata->RoomDemand);
    fpublishers[4]->setValue(locdata->WaterDemand);
    fpublishers[5]->setValue(locdata->Error);
    fpublishers[6]->setValue(le16toh(locdata->RoomTemperature));
    fpublishers[7]->setValue(le16toh(locdata->WaterTemperature));
    fpublishers[8]->setValue(le16toh(locdata->BatteryVoltage));
    FWaterDemand=locdata->WaterDemand;
    FWaterTemp=RawKelvinToTemp(le16toh(locdata->WaterTemperature));
}

TFrameSetTemp::TFrameSetTemp(uint8_t frameid)
{
    fframeid = frameid;
    setTemperature(0.0);
}

void TFrameSetTemp::setTemperature(double temp)
{
    ftemp = temp;
    //Serial.print("Setting temperature to ");
    //Serial.println(ftemp);
    TempToRawKelvin(ftemp,&fdata[0]);
}

TFrame34::TFrame34() : TFrameBase()
{
    fframeid=0x34;
    fpublishers.push_back(new TPubOperationTime("/operation_time"));
    fpublishers.push_back(new TPubBool("/k1"));
    fpublishers.push_back(new TPubBool("/k2"));
    fpublishers.push_back(new TPubBool("/k3"));
    fpublishers.push_back(new TPubEbtMode("/ebtmode"));
    fpublishers.push_back(new TPubHydronicStartInfo("/hydr_start_info"));
}

void TFrame34::publishFrameData()
{
    frame34Data *locdata=(frame34Data *) &fdata;
    fpublishers[0]->setValue(locdata->OperationTime[0]*65535+locdata->OperationTime[1]*256+locdata->OperationTime[2]);
    fpublishers[1]->setValue(locdata->K1);
    fpublishers[2]->setValue(locdata->K2);
    fpublishers[3]->setValue(locdata->K3);
    fpublishers[4]->setValue(locdata->EbtMode);
    fpublishers[5]->setValue(locdata->Event2);
}

TFrame37::TFrame37(): TFrameBase()
{
    fframeid = 0x37;
    fpublishers.push_back(new TPubFlameTemperature("/trend_value_hydronic"));
}

void TFrame37::publishFrameData()
{
    frame37Data *locdata=(frame37Data *) &fdata;
    fpublishers[0]->setValue(locdata->TrendValueHydronic);
}

TFrame39::TFrame39(): TFrameBase()
{
    fframeid=0x39;
    fpublishers.push_back(new TPubBlowOutTemperature("/blow_out_temp"));
    fpublishers.push_back(new TPubPumpFrequency("/pump_freq"));
    fpublishers.push_back(new TPubFlameTemperature("/flame_temp"));
}

void TFrame39::publishFrameData()
{
    frame39Data *locdata=(frame39Data *) &fdata;
    fpublishers[0]->setValue(le16toh(locdata->BlowOutTemperature));
    fpublishers[1]->setValue(locdata->PumpFrequency);
    fpublishers[2]->setValue(locdata->FlameTemperature);
}

TFrame35::TFrame35(): TFrameBase()
{
    fframeid = 0x35;
    fpublishers.push_back(new TPubBurnerFanVoltage("/burner_fan_v"));
    fpublishers.push_back(new TPubBurnerStatus("/burner_status"));
    fpublishers.push_back(new TPubGlowPlugStatus("/glow_plug_status"));
    fpublishers.push_back(new TPubHydronicState("/hydronic_state"));
    fpublishers.push_back(new TPubHydronicFlame("/hydronic_flame"));
}

void TFrame35::publishFrameData()
{
    frame35Data *locdata=(frame35Data *) &fdata;
    fpublishers[0]->setValue(locdata->BurnerFanVoltage);
    fpublishers[1]->setValue(locdata->AV3_Hydronic);
    fpublishers[2]->setValue(locdata->AV2_Hydronic);
    fpublishers[3]->setValue(locdata->EVENT0_Hydronic);
    fpublishers[4]->setValue(locdata->EVENT0_Hydronic);
}

TFrame3b::TFrame3b(): TFrameBase()
{
    fframeid = 0x3b;
    fpublishers.push_back(new TPubBattery("/battery"));
    fpublishers.push_back(new TPubExtractorFanRpm("/extractor_fan_rpm"));
    fpublishers.push_back(new TMqttPublisherBase("/current_error_hydronic"));
    fpublishers.push_back(new TPubPumpSafetySwitch("/pump_safety_switch"));
    fpublishers.push_back(new TMqttPublisherBase("/circ_air_motor_setpoint"));
    fpublishers.push_back(new TPubCircAirMotorCurrent("/circ_air_motor_current"));
}

void TFrame3b::publishFrameData()
{
    frame3bData *locdata=(frame3bData *) &fdata;
    fpublishers[0]->setValue(locdata->Battery);
    fpublishers[1]->setValue(locdata->ExtractorFanRpm);
    fpublishers[2]->setValue(locdata->CurrentErrorHydronic);
    fpublishers[3]->setValue(locdata->CurrentErrorHydronic);
    fpublishers[4]->setValue(locdata->CircAirMotor_Setpoint);
    fpublishers[5]->setValue(locdata->CircAirMotorCurrent);

}

TFrameSetFan::TFrameSetFan(uint8_t frameid)
{
    fframeid=frameid;
    setPumpOrFan(0x10);
}

void TFrameSetFan::setPumpOrFan(byte PumpOrFan)
{
    fPumpOrFan=PumpOrFan;
    fdata[0]=PumpOrFan | 0xe0;
    fdata[1]=0xfe;
}

TFrameEnergySelect::TFrameEnergySelect(uint8_t frameid)
{
    fframeid=frameid;
    setEnergySelection(EsGasDiesel);
}

void TFrameEnergySelect::setEnergySelection(TEnergySelection EnergySelection)
{
    TEnergyPriority priorities[] = {EpFuel, EpBothPrioFuel, EpBothPrioFuel, EpBothPrioElectro, EpBothPrioElectro};
    fEnergySelection=EnergySelection;
    fdata[0]=priorities[fEnergySelection];
}

TFrameSetPowerLimit::TFrameSetPowerLimit(uint8_t frameid)
{
    fframeid=frameid;
    setPowerLimit(EsGasDiesel);
}

void TFrameSetPowerLimit::setPowerLimit(TEnergySelection EnergySelection)
{
    uint16_t limits[] = {0,900,1800, 900,1800};
    fEnergySelection=EnergySelection;
    uint16_t limit=htole16(limits[fEnergySelection]);
    memcpy(&fdata[0],&limit,2);
}

TFrameSetControlElements::TFrameSetControlElements(uint8_t frameid)
{
    fframeid=frameid;
    SetSummerWinterMode(SWSummer60);
    SetElectroGasMix(GMGas);
    SetTempSetpoint(0);
    SetDiensteLin(0);
}

void TFrameSetControlElements::SetSummerWinterMode(TSummerWinterMode SummerWinterMode)
{
    fSummerWinterMode=SummerWinterMode;
    fdata[0] = fdata[0] & 0xf0 | fSummerWinterMode;
}

void TFrameSetControlElements::SetElectroGasMix(TElectroGasMixMode ElectroGasMixMode)
{
    fElectroGasMixMode=ElectroGasMixMode;
    fdata[0] = fdata[0] & 0x0f | (fElectroGasMixMode << 4);
}

void TFrameSetControlElements::SetTempSetpoint(uint16_t TempSetpoint)
{
    fTempSetpoint=TempSetpoint;
    uint16_t temp=htole16(fTempSetpoint);
    memcpy(&fdata[1],&temp,2);
}

void TFrameSetControlElements::SetDiensteLin(uint8_t DiensteLin)
{
    fDiensteLin=DiensteLin;
    fdata[2]=fDiensteLin;
}

TMasterFrame::TMasterFrame(uint8_t nad, uint8_t len, uint8_t sid ):TFrameBase()
{
   
   fenabled=true; 
   fnad=nad;
   flen=len;
   fsid=sid;
   fdata[0]=fnad;
   fdata[1]=flen; //here it's always single frame
   fdata[2]=fsid;
   for (int i=3; i<8; i++) {
    fdata[i]=0xff;
   }   
}

void TMasterFrame::setData(uint8_t *data)
{
    memcpy(freply, data,8);
    setReadResult(CheckReply());
}

void TMasterFrame::getData(uint8_t *dest)
{
    memcpy(dest, &fdata, 8);
}

void TMasterFrame::setReadResult(boolean ok)
{
    if (fdataok!=ok) {
        fdataok=ok;
        Serial.print("read master frame ");
        Serial.print(fsid,16);
        if (ok) {
            Serial.println(" ok");
        } else {
            Serial.println(" error");
        }
    }
}

bool TMasterFrame::CheckReply()
{
    return freply[2]==fsid+64;
}

uint8_t getProtectedID(uint8_t FrameID)
{
    // calc Parity Bit 0
    uint8_t p0 = bitRead(FrameID, 0) ^ bitRead(FrameID, 1) ^ bitRead(FrameID, 2) ^ bitRead(FrameID, 4);
    // calc Parity Bit 1
    uint8_t p1 = ~(bitRead(FrameID, 1) ^ bitRead(FrameID, 3) ^ bitRead(FrameID, 4) ^ bitRead(FrameID, 5));
    // combine bits to protected ID
    // 0..5 id is limited between 0x00..0x3F
    // 6    parity bit 0
    // 7    parity bit 1
    return ((p1 << 7) | (p0 << 6) | (FrameID & 0x3F));
}

TAssignFrameRanges::TAssignFrameRanges(uint8_t startindex, std::array<uint8_t,4> frames)
  :TMasterFrame(0x01, 0x06, 0xb7) 
{
    fdata[3]=startindex;
    for (int i=0; i<4; i++) {
        fdata[i+4]=getProtectedID(frames[i]);
    }
}

bool TOnOff::CheckReply()
{
    if (!TMasterFrame::CheckReply()) {
        return false;
    }
    frequestedstate = freply[3];
    fcurrentstate = freply[4];
    /*
    Serial.print("req.state ");
    Serial.print(frequestedstate,16);
    Serial.print(" current ");
    Serial.println(fcurrentstate, 16);
    */
    fpublishers[0]->setValue(frequestedstate);
    fpublishers[1]->setValue(fcurrentstate);
    return true;
}

TOnOff::TOnOff():TMasterFrame(0x01, 0x04, 0xb8)
{
    fdata[3]=0x10;
    fdata[4]=0x03;
    fpublishers.push_back(new TMqttPublisherBase("/requested_state"));
    fpublishers.push_back(new TMqttPublisherBase("/current_state"));
    SetOn(false);
}

void TOnOff::SetOn(bool on)
{
    fon=on;
    if (fon) {
        fdata[5]=0x01;
    } else {
        fdata[5]=0x00;
    }
}

TGetErrorInfo::TGetErrorInfo():TMasterFrame(0x01, 0x06, 0xb2)
{
    ferrorClass=0;
    ferrorCode=0;
    ferrorShort=0;
    fdata[3]=0x23;
    fdata[4]=0x17;
    fdata[5]=0x46;
    fdata[6]=0x10;
    fdata[7]=0x03;
    fpublishers.push_back(new TMqttPublisherBase("/err_class"));
    fpublishers.push_back(new TMqttPublisherBase("/err_code"));
    fpublishers.push_back(new TMqttPublisherBase("/err_short"));
}

bool TGetErrorInfo::CheckReply()
{
    if (!TMasterFrame::CheckReply()) {
     return false;
    }
    ferrorClass=freply[4];
    ferrorCode=freply[5];
    ferrorShort=freply[6];
    /*
    Serial.print("err.type ");
    Serial.print(ferrorClass,16);
    Serial.print(" code ");
    Serial.print(ferrorCode,16);
    Serial.print(" short ");
    Serial.println(ferrorShort,16);
    */
    fpublishers[0]->setValue(ferrorClass);
    fpublishers[1]->setValue(ferrorCode);
    fpublishers[2]->setValue(ferrorShort); 
    return true;
}
