#ifndef __GeoDateTimeServiceSIM_H__
#define __GeoDateTimeServiceSIM_H__
#include <memory>
#include "ISensor.h"

class GeoDateTimeServiceSIM : public ISensor<GeoDateTimeData>
{
  std::unique_ptr<GeoDateTimeData> m_DateTime = nullptr;

public:
  GeoDateTimeServiceSIM(const std::string &datetimestr)
      : m_DateTime(std::make_unique<GeoDateTimeData>(datetimestr)) {}

  ~GeoDateTimeServiceSIM() override {}

  int Initialize() override { return 0; }

  GeoDateTimeData GetServiceData() const override
  {
    return *m_DateTime;
  }
};

#endif // __GeoDateTimeServiceSIM_H__