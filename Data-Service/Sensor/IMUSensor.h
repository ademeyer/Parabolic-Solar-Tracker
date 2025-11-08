#pragma once
#include "ISensor.h"

class IMUSensor : public ISensor<IMUSensorData>
{
public:
  virtual int Initialize() override { return 0; }
  virtual Point3f Get3DAccelerometerData() const { return Point3f(-9.250, 0.307, 3.224); }
  virtual Point3f Get3DGyroscopeData() const { return Point3f(-101.4, -1.708, 70.732); }
  virtual Point3f Get3DMagneticData() const { return Point3f(-47.5, -21.1, -53.7); }
  IMUSensorData GetServiceData() const override
  {
    return IMUSensorData(Get3DAccelerometerData(), Get3DGyroscopeData(), Get3DMagneticData());
  }
};