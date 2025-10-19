#pragma once

#include "ISensor.h"
class IGeoLocationService : public ISensor<IGeoLocationService>
{
public:
  virtual int Initialize() override { return 0; }
  template <typename T>
  T GetServiceData() const
  {
    if constexpr (!std::is_same_v<T, GeoLocationData>)
      static_assert(sizeof(T) == 0, "Unsupported return type for IGeoLocationService");
    return GeoLocationData(51.047, -114.063, 1.181, 0.0);
  }
  virtual ~IGeoLocationService() override {}
};