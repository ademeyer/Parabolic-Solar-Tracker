#pragma once
#include "ISensor.h"

class IGeoDateTimeService : public ISensor<GeoDateTimeData>
{
public:
  ~IGeoDateTimeService() override {};

  int Initialize() override { return 0; }

  GeoDateTimeData GetServiceData() const override
  {
    std::time_t now = std::time(nullptr);
    std::tm *ltm = std::localtime(&now);
    return GeoDateTimeData(*ltm);
  }
};
