#pragma once

#include <cinttypes>
#include <string.h>
#include <endian.h>
#include <ESP32MQTTClient.h>
#include <array>
#include <vector>

/****************************************************
  Composition of frames read
*/

struct frame14Data
{
   uint16_t RoomTemperature;
   uint16_t RoomTargetTemperature;
   uint16_t WaterTargetTemperature;
   uint8_t EnergySelection;
   uint8_t Spare;
};

struct frame16Data
{
    uint8_t FixInfo;
    bool Antifreeze  :1;
    bool Supply220   :1;
    bool Window      :1;
    bool RoomDemand  :1;
    bool WaterDemand :1;
    bool bit13       :1;
    bool bit14       :1;
    bool Error       :1;
    uint16_t RoomTemperature;
    uint16_t WaterTemperature;
    uint16_t BatteryVoltage;
};

struct frame34Data
{
    uint8_t Event2;
    uint8_t OperationTime[3];
    uint8_t BtMode;
    uint8_t StatusFlags4;
    uint8_t EbtMode;
    bool K2    :1;
    bool K1    :1;
    bool K3    :1;
    bool bit3  :1;
    bool bit4  :1;
    bool bit5  :1;
    bool bit6  :1;
    bool bit7  :1;
};

struct frame37Data
{
    uint8_t StatusFlags2;
    uint8_t StatusFlags3;
    uint32_t DrawingNoHydronic;
    uint8_t ChangeLevelHydronic;
    uint8_t TrendValueHydronic; 
};

struct frame39Data
{
    int16_t BlowOutTemperature;
    int16_t BlowOutActualGradient;
    int16_t BlowOutGradient;
    uint8_t FlameTemperature;
    uint8_t PumpFrequency;
};

struct frame35Data
{
    uint8_t BatteryVoltageHydronic;
    uint8_t ConditionNrHydronic;
    uint8_t AV3_Hydronic;
    uint8_t EVENT0_Hydronic;
    uint8_t SafertyTime_Hydronic;
    uint8_t StateTime_Hydronic;
    uint8_t BurnerFanVoltage;
    uint8_t AV2_Hydronic;
};

struct frame3bData
{
    uint8_t SoftwareVersion_Tech;
    uint8_t CurrentErrorTruma;
    uint8_t CurrentErrorHydronic;
    uint8_t Battery;
    uint8_t Hyd_power_level;
    uint8_t CircAirMotor_Setpoint;
    uint8_t CircAirMotorCurrent;
    uint8_t ExtractorFanRpm;
};

/****************************************************
  Composition of frames written
*/
struct frameSetTemperatureData 
{
  uint16_t temperature;
  uint8_t empty[7];    
};

struct frameEnergySelectData
{
    uint8_t energypriority;
    uint8_t empty[7];
};

struct framePowerLimitData
{
    uint16_t powerLimit;
    uint8_t empty[6];
};

struct frameFanModeData
{
    uint8_t fanmode;
    uint8_t fe;
    uint8_t empty[6];
};


/****************************************************
  Base class for all values to be sent to the mqtt broker
  each derived class will override the getPayload method
  to transform the internal value (always a dword already
  converted to the correct endiannes) to a suitable
  representation
*/
class TMqttPublisherBase {
  private:
    String ftopic;
    unsigned long flastsent;
    boolean fforcesend;
  protected:
    uint32_t fvalue;
    virtual String getPayload() {
        return String(fvalue);
    };
  public:
    TMqttPublisherBase(String topic);
    void setValue(uint32_t newvalue);
    void setForcesend();
};

/****************************************************
  Base class for all frames
*/
class TFrameBase {
    protected:
        uint8_t fdata[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        uint8_t fframeid;
        boolean fdataok;
        std::vector<TMqttPublisherBase*> fpublishers;
    public:
        TFrameBase();
        virtual void setReadResult(boolean ok);
        virtual void setData(uint8_t *data);
        void setForcesend();
        void getData(uint8_t *dest);
        //virtual method called after setData, 
        //each derived class will use it to get the values from the frame
        //and report back to the mqtt broker/websocket clients
        virtual void publishFrameData();
        uint8_t frameid() {return fframeid;};
        boolean getDataOk() {return fdataok;};          
};

/* conversion functions */
double RawKelvinToTemp(const uint16_t RawValue);
void TempToRawKelvin(double temp, uint8_t *dest);
double RawToVoltage(const uint16_t RawValue);
double RawToFlameTemperature(const uint8_t RawValue);


/* publishes a temperature in ºC*/
class TPubTemperature: public TMqttPublisherBase {
protected:   
   virtual String getPayload() override {
    return String(RawKelvinToTemp(fvalue),1);
   };
public:
  TPubTemperature(String topic) : TMqttPublisherBase(topic) {}  
};

/* publshes a boolean value as 0 or 1 */
class TPubBool: public TMqttPublisherBase {
protected:   
   virtual String getPayload() override {
    return String(fvalue);
   };
public:
  TPubBool(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes a voltage */
class TPubVoltage: public TMqttPublisherBase {
protected:   
   virtual String getPayload() override {
    return String(RawToVoltage(fvalue),1);
   };
public:
  TPubVoltage(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the current operating mode */
class TPubEbtMode: public TMqttPublisherBase {
protected:
  virtual String getPayload() override {
    switch (fvalue) {
      case 0: return "Electric 1800W";
      case 1: return "Electric 900W";
      case 2: return "Gas/Diesel";
      case 3: return "Mixed 900W";
      case 4: return "Mixed 1800W";
    }
    return "?"+String(fvalue);
  }  
public:
  TPubEbtMode(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the start information of the eberspacher burner */
class TPubHydronicStartInfo: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    uint8_t b = fvalue & 0x07;
    switch(b) {
      case 0: return "No start info";
      case 1: return "1st start";
      case 2: return "Normal start";
      case 4: return "Repeat start";
    }
    return "?"+String(b,16);
  }
public:
  TPubHydronicStartInfo(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the operational time in minutes */
class TPubOperationTime: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    return String(fvalue); //FIXME, maybe convert to hours?
  }
public:
  TPubOperationTime(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the blow out temperature in ºC*/
class TPubBlowOutTemperature: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    uint16_t value=fvalue;
    return String(value*0.1,1);
  }
public:
  TPubBlowOutTemperature(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the flame temperature in ºC*/
class TPubFlameTemperature: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    return String(RawToFlameTemperature(fvalue),0);
  }
public:
  TPubFlameTemperature(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the diesel pump frequency in Hz*/
class TPubPumpFrequency: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    double freq=fvalue/25.0;
    return String(freq,2);
  }
public:
  TPubPumpFrequency(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the burner fan voltage in V*/
class TPubBurnerFanVoltage: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    return String(fvalue*0.1,1);
  }
public:
  TPubBurnerFanVoltage(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the burner status*/
class TPubBurnerStatus: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    uint8_t b=fvalue & 0x2f;
    switch (b) {
      case 0: return "no info";
      case 1: return "blow cold";
      case 2: return "level high";
      case 4: return "level middle";
      case 8: return "level low";
      case 0x10: return "system check";
      case 0x20: return "run after";
    }
    return "?"+String(b,16);
  }
public:
  TPubBurnerStatus(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes if the eberspacher burner flame is on or off*/
class TPubHydronicFlame: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    uint8_t b=fvalue & 2;
    if (b==0) {
      return "off";
    } else {
      return "on";
    }
  }
public:
  TPubHydronicFlame(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the eberspacher burner state */
class TPubHydronicState: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    uint8_t b=fvalue & 0x0c;
    switch(b) {
      case 0: return "no voltage";
      case 4: return "start";
      case 8: return "restart";
    }
    return "?"+String(b,16);
  }
public:
  TPubHydronicState(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the glow plug status */
class TPubGlowPlugStatus: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    uint8_t b=fvalue & 0x0c;
    switch (b) {
      case 0: return "no voltage";
      case 4: return "start";
      case 8: return "restart";
    }
    return "?"+String(b,16);

  }
public:
  TPubGlowPlugStatus(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the battery voltage received in frame 3b*/
class TPubBattery: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    return String(fvalue * 0.1, 1);
  }
public:
  TPubBattery(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the circulating air motor current */
class TPubCircAirMotorCurrent: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    return String(fvalue * 0.1, 1);
  }
public:
  TPubCircAirMotorCurrent(String topic) : TMqttPublisherBase(topic) {}  
};

/* publishes the extractor fan rpm */
class TPubExtractorFanRpm: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    if (fvalue==0) {
      return "0";
    }
    double result = round(58594.0 / fvalue);
    if (result<260) {
      return "0";
    }
    return String(result,0);
  }
public:
  TPubExtractorFanRpm(String topic) : TMqttPublisherBase(topic) {}  
};

/* pump safety switch open or closed */
class TPubPumpSafetySwitch: public TMqttPublisherBase {
  protected:
  virtual String getPayload() override {
    if (fvalue==48) {
      return "open";
    } else {
      return "closed";
    }
  }
public:
  TPubPumpSafetySwitch(String topic) : TMqttPublisherBase(topic) {}  
};

/*****************************************************
 * classes for frames read
*/

class TFrame14: public TFrameBase {
    public:
      TFrame14();
      void publishFrameData() override;
};

class TFrame16: public TFrameBase {
    private:
       bool FWaterDemand;
       double FWaterTemp;
    public:
      TFrame16();
      void publishFrameData() override;
      //method to keep track of current water demand and temperature
      //for the  
      bool getWaterDemand() { return FWaterDemand;};
      double getWaterTemp() { return FWaterTemp;}; 
};

class TFrame34: public TFrameBase {
   public:
     TFrame34();
     void publishFrameData() override;  
};

class TFrame37: public TFrameBase {
   public:
     TFrame37();
     void publishFrameData() override;  
};

class TFrame39: public TFrameBase {
   public:
     TFrame39();
     void publishFrameData() override;  
};

class TFrame35: public TFrameBase {
   public:
     TFrame35();
     void publishFrameData() override;  
};

class TFrame3b: public TFrameBase {
   public:
     TFrame3b();
     void publishFrameData() override;  
};

/********************************************************
 * classes for frames to be written
*/

/* used in frames 
   0x02 simulated room temperature
   0x03 room temperature setpoint
   0x04 water temperature setpoint
*/
class TFrameSetTemp: public TFrameBase {
    private:
      double ftemp;
    public:
      TFrameSetTemp(uint8_t frameid);
      void setTemperature(double temp);
};

class TFrameSetFan: public TFrameBase {
    private:
      byte fPumpOrFan;
    public:
      TFrameSetFan(uint8_t frameid);
      void setPumpOrFan(byte PumpOrFan);
};

enum TEnergySelection {EsGasDiesel, EsMixed900, EsMixed800, EsElectro900, EsElectro1800};
enum TEnergyPriority {EPNone=0, EpFuel, EpBothPrioElectro, EpBothPrioFuel};

class TFrameEnergySelect: public TFrameBase {
  private:
    TEnergySelection fEnergySelection;
  public:
    TFrameEnergySelect(uint8_t frameid);
    void setEnergySelection(TEnergySelection EnergySelection); 
};

class TFrameSetPowerLimit: public TFrameBase {
  private:
    TEnergySelection fEnergySelection;
  public:
    TFrameSetPowerLimit(uint8_t frameid);
    void setPowerLimit(TEnergySelection EnergySelection); 
};

/*******************************************************************
 * Master Request/Slave reply frames
 * 
*/
class TMasterFrame: public TFrameBase {
  protected:
   uint8_t freply[8];
   uint8_t fnad;
   uint8_t flen;
   uint8_t fsid;
   boolean fenabled; 
   virtual bool CheckReply();
  public:
    TMasterFrame(uint8_t nad, uint8_t len, uint8_t sid);
    virtual void setData(uint8_t *data) override;
    void getData(uint8_t *dest);
    virtual void setReadResult(boolean ok);
    void setEnabled(boolean value) {fenabled=value;};
    boolean getEnabled() { return fenabled;};
};

// frame to enable frames not usually availavble
class TAssignFrameRanges: public TMasterFrame {
  protected:
  public:
    TAssignFrameRanges(uint8_t startindex, std::array<uint8_t,4> frames);
};

// frame to turn on/off and get the current state
class TOnOff: public TMasterFrame {
  private:
    bool fon;
    uint8_t frequestedstate;
    uint8_t fcurrentstate;
  protected:
    virtual bool CheckReply() override;  
  public:
    TOnOff();  
    void SetOn(bool on);
    boolean GetOn() { return fon;};
    uint8_t getRequestedState() { return frequestedstate;};
    uint8_t getCurrentState() { return fcurrentstate;};
};

// frame to get the error information
class TGetErrorInfo: public TMasterFrame {
  private:
    uint8_t ferrorClass;
    uint8_t ferrorCode;
    uint8_t ferrorShort;
  protected:
    virtual bool CheckReply() override;
  public:
    TGetErrorInfo();
    uint8_t getErrorClass() { return ferrorClass;};
    uint8_t getErrorCode() { return ferrorCode;};
    uint8_t getErrorShorw() { return ferrorShort;};
};