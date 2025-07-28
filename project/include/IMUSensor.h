#pragma once
#include "ISensor.h"
#include "Point.h"

struct IMUSensorData
{
  Point3f accel; /* X, Y, Z (m/s^2) */
  Point3f gyro;  /* Yaw, Pitch, Roll (degree) */
  Point3f mag;   /* X, Y, Z (uT) */
  IMUSensorData() : accel(Point3f()), gyro(Point3f()), mag(Point3f()) {}
  IMUSensorData(const Point3f &a, const Point3f &g, const Point3f &m)
      : accel(a), gyro(g), mag(m) {}
};

class IMUSensor : public ISensor
{
public:
  virtual int Initialize() override { return 0; }
  virtual void GetRawSensorData() override {}
  virtual Point3f Get3DAccelerometerData() const { return Point3f(-9.250, 0.307, 3.224); }
  virtual Point3f Get3DGyroscopeData() const { return Point3f(-101.4, -1.708, 70.732); }
  virtual Point3f Get3DMagneticData() const { return Point3f(-47.5, -21.1, -53.7); }
  virtual IMUSensorData GetIMUSensorData() const { return IMUSensorData(Get3DAccelerometerData(), Get3DGyroscopeData(), Get3DMagneticData()); }
};