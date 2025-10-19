#pragma once
#include "ISensor.h"

class IGeoWeatherService : public ISensor<IGeoWeatherService>
{
public:
  virtual int Initialize() override { return 0; }
  template <typename T>
  T GetServiceData() const
  {
    if constexpr (!std::is_same_v<T, GeoWeatherData>)
      static_assert(sizeof(T) == 0, "Unsupported return type for IGeoWeatherService");
    return GeoWeatherData(18.0, 895.0, 56.0);
  }
};