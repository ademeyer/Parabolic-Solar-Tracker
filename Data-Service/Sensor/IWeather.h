#pragma once
#include "ISensor.h"

class IGeoWeatherService : public ISensor<GeoWeatherData>
{
public:
  int Initialize() override { return 0; }
  GeoWeatherData GetServiceData() const
  {
    return GeoWeatherData(18.0, 895.0, 56.0, 0.0);
  }
};