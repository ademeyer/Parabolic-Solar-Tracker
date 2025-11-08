#pragma once

#include "ISensor.h"
class IGeoLocationService : public ISensor<GeoLocationData>
{
public:
  int Initialize() override { return 0; }

  GeoLocationData GetServiceData() const
  {
    return GeoLocationData(51.047, -114.063, 1.181, 0.0);
  }
  virtual ~IGeoLocationService() override {}
};