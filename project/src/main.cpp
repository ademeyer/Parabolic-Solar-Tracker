#include <math.h>
#include "WMMLib.h"
#include <iostream>

int main()
{
  // Get 3D magnet data
  IMUSensor imu;
  Point3f mag = imu.Get3DMagneticData();
  // Calculate the magnetic north direction (θ mag)
  double theta = atan(mag.Y / mag.X);
  // Configure Input to get local north direction
  IDateTime dt;
  IGPSSensor gps;
  InData in;
  in.decimalYear = dt.GetDecimalYear();
  in.pos = gps.GetPositionData();
  // Get local magnetic north direction (D)
  auto decl = getDeclinition(&in);
  if (decl.errCode != 0)
  {
    std::cerr << "An Error occurred: " << decl.errCode << " While trying to get local declinition" << std::endl;
    return 1;
  }
  // Calculate true north position releative to mag sensor (θ true = θ mag - D)
  auto trueNorth = theta - decl.magData.D;
  // Print our result to console
  std::cout << "Magnetic Sensor North: " << theta
            << ", Local Declinition: " << decl.magData.D
            << ", Local Declinition Error: " << decl.magDataErr.D
            << ", True North: " << trueNorth << " due "
            << (trueNorth > 0 ? "East" : "West") << std::endl;
  return 0;
}