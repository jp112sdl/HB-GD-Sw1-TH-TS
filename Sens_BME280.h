
#ifndef _SENS_BME280_H_
#define _SENS_BME280_H_

<<<<<<< HEAD
#include <Wire.h>
#include <Sensors.h>
#include <BME280I2C.h>
=======


#include <Sensors.h>
#include <BME280Spi.h>
>>>>>>> 2e1768f (initial commit)
#include <EnvironmentCalculations.h>

namespace as {

<<<<<<< HEAD
BME280I2C::Settings settings(
   BME280::OSR_X1, // Temperature Oversampling Rate (tempOSR): OSR Enum, default = OSR_X1
   BME280::OSR_X1, // Humidity Oversampling Rate (humOSR): OSR Enum, default = OSR_X1
   BME280::OSR_X1, // Pressure Oversampling Rate (presOSR): OSR Enum, default = OSR_X1
   BME280::Mode_Forced, // Mode (mode): Mode Enum, default = Mode_Forced
   BME280::StandbyTime_1000ms, // Standby Time (standbyTime): StandbyTime Enum, default = StandbyTime_1000ms
   BME280::Filter_Off, // Filter (filter): Filter Enum, default = Filter_16
   BME280::SpiEnable_False // SPI Enable: SpiEnable Enum, default = false
);
=======
BME280Spi::Settings settings(5);
>>>>>>> 2e1768f (initial commit)

class Sens_Bme280 : public Sensor {

  int16_t   _temperature;
  uint16_t  _pressure;
  uint16_t  _pressureNN;
  uint8_t   _humidity;
  uint16_t  _dewPoint;

<<<<<<< HEAD
  BME280I2C _bme280;        // Default : forced mode, standby time = 1000 ms, Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off

public:

  Sens_Bme280 () : _temperature(0), _pressure(0), _pressureNN(0), _humidity(0), _dewPoint(0) {}

  void init () {

    Wire.begin(); //ToDo sync with further I2C sensor classes

=======
  BME280Spi _bme280;        // Default : forced mode, standby time = 1000 ms, Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off

public:

  Sens_Bme280 () : _temperature(0), _pressure(0), _pressureNN(0), _humidity(0), _dewPoint(0), _bme280(settings) {}

  void init () {
    SPI.begin();
>>>>>>> 2e1768f (initial commit)
    uint8_t i = 10;
    while( (!_bme280.begin()) && (i > 0) ) {
      delay(100);
      i--;
    }
<<<<<<< HEAD
  
=======

>>>>>>> 2e1768f (initial commit)
    switch(_bme280.chipModel()) {
     case BME280::ChipModel_BME280:
       _present = true;
       DPRINTLN(F("BME280 sensor OK"));
       break;
     default:
       DPRINTLN(F("BME280 sensor NOT OK"));
    }
<<<<<<< HEAD
    
    _bme280.setSettings(settings);
=======

>>>>>>> 2e1768f (initial commit)
  }

  void measure (uint16_t height) {
    if (_present == true) {
      float temp(NAN), hum(NAN), pres(NAN);
      _bme280.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
      
      _temperature = (int16_t)(temp * 10);
      _pressure    = (uint16_t)(pres * 10);
      _pressureNN  = (uint16_t)(EnvironmentCalculations::EquivalentSeaLevelPressure(float(height), temp, pres) * 10);
      _humidity    = (uint8_t)hum;
      _dewPoint    = (int16_t)(EnvironmentCalculations::DewPoint(temp, hum, EnvironmentCalculations::TempUnit_Celsius) * 10);
      
      DPRINTLN(F("BME280:"));
      DPRINT(F("-T    : ")); DDECLN(_temperature);
      DPRINT(F("-P    : ")); DDECLN(_pressure);
      DPRINT(F("-P(NN): ")); DDECLN(_pressureNN);
      DPRINT(F("-H    : ")); DDECLN(_humidity);
      DPRINT(F("-DP   : ")); DDECLN(_dewPoint);
    }
  }
  
  int16_t  temperature () { return _temperature; }
  uint16_t pressure ()    { return _pressure; }
  uint16_t pressureNN ()  { return _pressureNN; }
  uint8_t  humidity ()    { return _humidity; }
  int16_t  dewPoint ()    { return _dewPoint; }
};

}

#endif
