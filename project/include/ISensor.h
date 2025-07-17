#pragma once
class ISensor
{
public:
  virtual int Initialize() = 0;
  virtual void GetRawSensorData() = 0;
};