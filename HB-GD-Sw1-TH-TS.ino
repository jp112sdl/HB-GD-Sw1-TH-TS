//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-08-19 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// ci-test=yes board=mega128 aes=no
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER


#define HIDE_IGNORE_MSG

#ifndef __AVR_ATmega128__
  #define EI_NOTEXTERNAL
  #include <EnableInterrupt.h>
  #include <LowPower.h>
#endif
#include <SPI.h>
#include <AskSinPP.h>
#include <sensors/Aht1x.h>

#include <Register.h>
#include <MultiChannelDevice.h>
#include <ContactState.h>
#include <Switch.h>

#ifdef __AVR_ATmega128__
  #define CONFIG_BUTTON_PIN   7 //PE7
  #define LED_PIN            22 //PD4
  #define CC1101_CS           8 //PB0
  #define CC1101_GDO0         6 //PE6

  #define BTN1_PIN           5 //PE5
  #define TS_PIN1            3 //PE3
  #define TS_PIN2            4 //PE4
  #define RELAY_PIN         15 //PB7

#endif

#define INVERT_RELAY      false
#define PEERS_PER_CHANNEL 8
#define MSG_INTERVAL      180

using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0xf3,0x5c,0x01},       // Device ID
    "JPGD000001",           // Device Serial
    {0xf3,0x5c},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::Switch, // Device Type
    {0x01,0x00}             // Info Bytes
};


typedef LibSPI<CC1101_CS> RadioSPI;
typedef AskSin<StatusLed<LED_PIN>,NoBattery,Radio<RadioSPI,CC1101_GDO0> > Hal;
Hal hal;

DEFREGISTER(Reg0,DREG_INTKEY,MASTERID_REGS, DREG_TRANSMITTRYMAX)
class GDList0 : public RegList0<Reg0> {
public:
  GDList0(uint16_t addr) : RegList0<Reg0>(addr) {}

  void defaults () {
    clear();
    intKeyVisible(true);
    transmitDevTryMax(6);
  }
};

DEFREGISTER(Reg1, CREG_AES_ACTIVE, CREG_MSGFORPOS, CREG_EVENTDELAYTIME, CREG_LEDONTIME)
class SCList1 : public RegList1<Reg1> {
  public:
    SCList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
    void defaults () {
      clear();
      msgForPosA(1);
      msgForPosB(2);
      msgForPosC(3);
      aesActive(false);
      eventDelaytime(0);
      ledOntime(100);
      transmitTryMax(6);
    }
};
typedef ThreeStateChannel<Hal, GDList0, SCList1, DefList4, PEERS_PER_CHANNEL> SCChannel;
typedef SwitchChannel<Hal, PEERS_PER_CHANNEL, GDList0>  SWChannel;

DEFREGISTER(WeaReg1, 0x01, 0x02, 0x03, 0x04, 0x05)
class WeaList1 : public RegList1<WeaReg1> {
  public:
  WeaList1 (uint16_t addr) : RegList1<WeaReg1>(addr) {}

    bool TemperatureOffset (int32_t value) const {
      return
          this->writeRegister(0x01, (value >> 24) & 0xff) &&
          this->writeRegister(0x02, (value >> 16) & 0xff) &&
          this->writeRegister(0x03, (value >> 8)  & 0xff) &&
          this->writeRegister(0x04, (value)       & 0xff)
          ;
    }

    int32_t TemperatureOffset () const {
      return
          ((int32_t)(this->readRegister(0x01, 0)) << 24) +
          ((int32_t)(this->readRegister(0x02, 0)) << 16) +
          ((int32_t)(this->readRegister(0x03, 0)) << 8 ) +
          ((int32_t)(this->readRegister(0x04, 0))      )
          ;
    }

    int8_t HumidityOffset () const { return this->readRegister(0x05,0); }
    bool HumidityOffset (int8_t value) const { return this->writeRegister(0x05,value); }

    void defaults () {
      clear();
      TemperatureOffset(0);
      HumidityOffset(0);
    }
};

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temp, uint8_t humidity) {
      uint8_t t1 = (temp >> 8) & 0x7f;
      uint8_t t2 = temp & 0xff;
      Message::init(0xc, msgcnt, 0x70, BIDI | WKMEUP, t1, t2);
      pload[0] = humidity;
    }
};
class WeatherChannel : public Channel<Hal, WeaList1, EmptyList, List4, PEERS_PER_CHANNEL, GDList0>, public Alarm {

    WeatherEventMsg msg;
    int16_t         temp;
    uint8_t         humidity;

    Aht1x<>         aht15;
    uint16_t        millis;

  public:
    WeatherChannel () : Channel(), Alarm(5), temp(0), humidity(0), millis(0) {}
    virtual ~WeatherChannel () {}


    // here we do the measurement
    void measure () {
      DPRINT("Measure...");
      aht15.measure();

      int32_t OFFSETtemp = this->getList1().TemperatureOffset();
      int32_t OFFSEThumi = this->getList1().HumidityOffset();

      temp = aht15.temperature() + OFFSETtemp;
      humidity = aht15.humidity() + OFFSEThumi;

      DPRINTLN("T/H = " + String(temp) + "/" + String(humidity));
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);
      measure();
      msg.init(msgcnt, temp, humidity);
      device().broadcastEvent(msg);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, GDList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      aht15.init();
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class UType : public ChannelDevice<Hal, VirtBaseChannel<Hal, GDList0>, 3, GDList0> {
  public:
    VirtChannel<Hal, WeatherChannel, GDList0>    channel1;
    VirtChannel<Hal, SCChannel, GDList0>         channel2;
    VirtChannel<Hal, SWChannel, GDList0>         channel3;
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, GDList0>, 3, GDList0> DeviceType;

    UType (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(channel1, 1);
      DeviceType::registerChannel(channel2, 2);
      DeviceType::registerChannel(channel3, 3);
    }
    virtual ~UType () {}

    WeatherChannel& weaChannel() { return channel1; }
    SCChannel&      tsChannel () { return channel2; }

    SWChannel&      swChannel () { return channel3; }


    virtual void configChanged () {
      DeviceType::configChanged();
    }
};

UType sdev(devinfo, 0x20);

ConfigButton<UType> cfgBtn(sdev);
InternalButton<UType> btn1(sdev,1);

void initPeerings (bool first) {
  if( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    Peer ipeer(devid,1);
    sdev.swChannel().peer(ipeer);
  }
}

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);

  sdev.swChannel().init(RELAY_PIN, INVERT_RELAY);

  const uint8_t posmap[4] = {Position::State::PosB,Position::State::PosA,Position::State::PosB,Position::State::PosC};
  sdev.tsChannel().init(TS_PIN1, TS_PIN2, posmap);

  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  buttonISR(btn1, BTN1_PIN)
  initPeerings(first);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
#ifndef __AVR_ATmega128__
    hal.activity.savePower<Sleep<> >(hal);
#endif
   }
}

