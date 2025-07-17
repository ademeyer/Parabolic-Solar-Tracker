#pragma once

#include "ISensor.h"

struct Position
{
  double Latitude;
  double Longitude;
  double Altitude;
  double Speed;
  Position() : Latitude(0.0), Longitude(0.0), Altitude(0.0), Speed(0.0) {}
  Position(const double &lat, const double &lon, const double &alt, const double &speed)
      : Latitude(lat), Longitude(lon), Altitude(alt), Speed(speed) {}
};

class IGPSSensor : public ISensor
{
public:
  virtual int Initialize() override { return 0; }
  virtual void GetRawSensorData() override {}
  virtual Position GetPositionData() const { return Position(51.047, -114.063, 1.181, 0.0); }
};