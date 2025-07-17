#pragma once
#include "ISensor.h"
#include "Point.h"

struct IMUSensorData
{
  Point3f accel;
  Point3f gyro;
  Point3f mag;
  IMUSensorData() : accel(Point3f()), gyro(Point3f()), mag(Point3f()) {}
};

class IMUSensor : public ISensor
{
public:
  virtual int Initialize() override { return 0; }
  virtual void GetRawSensorData() override {}
  virtual Point3f Get3DAccelerometerData() const { return Point3f(); }
  virtual Point3f Get3DGyroscopeData() const { return Point3f(); }
  virtual Point3f Get3DMagneticData() const { return Point3f(-47.5, -21.1, -53.7); }
  virtual IMUSensorData GetIMUSensorData() const { return IMUSensorData(); }
};