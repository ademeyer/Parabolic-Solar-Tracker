#pragma once
#include "ISensor.h"

class IMUSensor : public ISensor<IMUSensor>
{
public:
  virtual int Initialize() override { return 0; }
  virtual Point3f Get3DAccelerometerData() const { return Point3f(-9.250, 0.307, 3.224); }
  virtual Point3f Get3DGyroscopeData() const { return Point3f(-101.4, -1.708, 70.732); }
  virtual Point3f Get3DMagneticData() const { return Point3f(-47.5, -21.1, -53.7); }
  template <typename T>
  T GetServiceData() const
  {
    if constexpr (!std::is_same_v<T, IMUSensorData>)
      static_assert(sizeof(T) == 0, "Unsupported return type for IMUSensorData");
    return IMUSensorData(Get3DAccelerometerData(), Get3DGyroscopeData(), Get3DMagneticData());
  }
};