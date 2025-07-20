#pragma once
#include "ISensor.h"

struct WeatherData
{
  double temp;     /* in Celsius */
  double presure;  /* in millibar */
  double humidity; /* in % */
  WeatherData(const double &t, const double &p, const double &h)
      : temp(t), presure(p), humidity(h) {}
};

class IWeather : public ISensor
{
public:
  virtual int Initialize() override { return 0; }
  virtual void GetRawSensorData() override {}
  virtual WeatherData GetWeatherData() const
  {
    return WeatherData(18.0, 895.0, 56.0);
  }
};