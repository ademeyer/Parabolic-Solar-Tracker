#pragma once

struct WeatherData
{
  double temp;
  double presure;
  double humidity;
  WeatherData(const double &t, const double &p, const double &h)
      : temp(t), presure(p), humidity(h) {}
};

class IWeather
{
public:
  virtual WeatherData GetWeatherData() const
  {
    return WeatherData(18.0, 895.0, 56.0);
  }
};