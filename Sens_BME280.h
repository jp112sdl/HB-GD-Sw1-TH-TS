
#ifndef _SENS_BME280_H_
#define _SENS_BME280_H_

#include <Sensors.h>
#include <BME280Spi.h>
#include <EnvironmentCalculations.h>

namespace as {

BME280Spi::Settings settings(5);

class Sens_Bme280 : public Sensor {

  int16_t   _temperature;
  uint16_t  _pressure;
  uint16_t  _pressureNN;
  uint8_t   _humidity;
  uint16_t  _dewPoint;


  BME280Spi _bme280;        // Default : forced mode, standby time = 1000 ms, Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off

public:

  Sens_Bme280 () : _temperature(0), _pressure(0), _pressureNN(0), _humidity(0), _dewPoint(0), _bme280(settings) {}

  void init () {
    SPI.begin();
    uint8_t i = 10;
    while( (!_bme280.begin()) && (i > 0) ) {
      delay(100);
      i--;
    }

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
