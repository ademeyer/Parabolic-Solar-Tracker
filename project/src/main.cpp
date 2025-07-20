#include <math.h>
#include "WMMLib.h"
#include "SPALib.h"
#include <iostream>

int main()
{
  // Get 3D magnet data
  IMUSensor imu;
  Point3f mag = imu.Get3DMagneticData();
  // Calculate the magnetic north direction (θ mag)
  double theta = atan(mag.Y / mag.X);
  IDateTime dt;
  IGPSSensor gps;
  InData in;
  IWeather weather;
  SPA_Input spa_in(dt.GetDateTimeDate(), gps.GetPositionData(), weather.GetWeatherData());
  // Configure Input to get local north direction
  in.decimalYear = dt.GetDecimalYear();
  in.pos = gps.GetPositionData();
  // Get local magnetic north direction (D)
  auto decl = getDeclinition(&in);
  if (decl.errCode != 0)
  {
    std::cerr << "An Error occurred: " << decl.errCode << " While trying to get local declinition" << std::endl;
    return 1;
  }
  // https://www.suncalc.org/#/51.0108,-114.0646,18/2025.07.20/09:29/1/3
  // Calculate true north position releative to mag sensor (θ true = θ mag - D)
  auto trueNorth = theta - decl.magData.D;
  // Get Sun position
  auto spa_data = getSunPosition(&spa_in);
  if (spa_data.errCode != 0)
  {
    std::cerr << "An Error occurred: " << spa_data.errCode << " While trying to get sun position" << std::endl;
    return 1;
  }
  // Print result out to console
  std::cout
      << "Magnetic Sensor North [degrees]: " << theta
      << "\nLocal Declinition [degrees]: " << decl.magData.D
      << "\nLocal Declinition Error [degrees]: " << decl.magDataErr.D
      << "\nTrue North [degrees]: " << trueNorth << " due " << (trueNorth > 0 ? "East" : "West")
      << "\nAzimuth [degrees]: " << spa_data.azimuth
      << "\nElevation [degrees]: " << spa_data.elevation
      << "\nIncidence [degrees]: " << spa_data.incidence
      << "\nSunrise: " << spa_data.sunrise.hour << ":" << spa_data.sunrise.minute << ":" << spa_data.sunrise.second << " localtime"
      << "\nSunset: " << spa_data.sunset.hour << ":" << spa_data.sunset.minute << ":" << spa_data.sunset.second << " localtime"
      << std::endl;
  return 0;
}