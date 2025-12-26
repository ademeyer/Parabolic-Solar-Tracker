#include <iostream>
#include "GeoLocationServiceSIM.hpp"
#include "GeoDateTimeServiceSIM.hpp"
#include "GeoWeatherServiceSIM.hpp"
#include "GeoSolarRadiationServiceSIM.hpp"
#include "WMMLib.h"
#include "SPALib.h"

// General Print Helper Function
template <typename T>
void PrintISensorData(const T &data)
{
  if constexpr (std::is_same_v<T, GeoLocationData>)
  {
    std::cout << "====== GeoLocationData ======\n"
              << "Latitute [degrees]: " << data.Latitude << std::endl
              << "Longitude [degrees]: " << data.Longitude << std::endl
              << "Altitude [meter]: " << data.Altitude << std::endl
              << "Speed [m/s]: " << data.Speed << std::endl;
  }
  else if constexpr (std::is_same_v<T, GeoWeatherData>)
  {
    std::cout << "====== GeoWeatherData ======\n"
              << "Temperature [deg. Celsius]: " << data.temp << std::endl
              << "Pressure [milliBar]: " << data.pressure << std::endl
              << "Humidity [%]: " << data.humidity << std::endl
              << "Wind Speed [m/s]: " << data.wind_speed << std::endl;
  }
  else if constexpr (std::is_same_v<T, GeoDateTimeData>)
  {
    std::cout << "====== GeoDateTimeData ======\n"
              << (data.dt.date.year) << "-" << data.dt.date.month << "-" << data.dt.date.day
              << " " << data.dt.time.hour << ":" << data.dt.time.minute << ":" << data.dt.time.second
              << "TZ" << data.dt.time.timezone << std::endl;
  }
  else if constexpr (std::is_same_v<T, GeoMagneticData>)
  {
    std::cout << "====== GeoMagneticData ======\n"
              << "Local Magnetic North [degrees]: " << data.GetLocalMagneticNorth() << std::endl
              << "Geo Magnetic North (True North) [degrees]: " << data.GetGeoMagneticNorth() << std::endl
              << "Declinition [degrees]: " << data.magData.D << std::endl;
  }
  else if constexpr (std::is_same_v<T, SunPositionData>)
  {
    std::cout << "====== SunPositionData ======\n"
              << "Azimuth [degrees]: " << data.azimuth << std::endl
              << "Elevation [degrees]: " << data.elevation << std::endl
              << "Incidence [degrees]: " << data.incidence << std::endl
              << "Sunrise [local time]: " << data.sunrise.hour << ":" << data.sunrise.minute << ":" << data.sunrise.second << std::endl
              << "Sunset [local time]: " << data.sunset.hour << ":" << data.sunset.minute << ":" << data.sunset.second << std::endl;
  }
  else if constexpr (std::is_same_v<T, GeoSolarRadiationData>)
  {
    std::cout << "====== GeoSolarRadiationData ======\n"
              << "DNI [W/m²]: " << data.DNI << std::endl
              << "DHI [W/m²]: " << data.DHI << std::endl
              << "GHI [W/m²]: " << data.GHI << std::endl
              << "DHH [W/m²]: " << data.DHH << std::endl;
  }
  else
  {
    static_assert(sizeof(T) == 0, "Unknown ISensor Data type");
  }
}

int main()
{
  /* Get GeoLocationData for addresses declared in sim.conf, or declare another conf file */
  std::unique_ptr<GeoLocationServiceSIM> gService = std::make_unique<GeoLocationServiceSIM>();
  if (gService->Initialize() != 0)
  {
    std::cerr << "Failed to Initialize Geo Location Service" << std::endl;
    return EXIT_FAILURE;
  }
  auto address_loc = gService->GetServiceData();
  for (const auto &[addr, gLoc] : address_loc)
  {
    std::cout << "***************************************************" << addr << "******************************************************" << std::endl;
    PrintISensorData(gLoc);
    if (!gLoc)
    {
      std::cout << "Invalid Geo Location Data for " << addr << std::endl;
      continue;
    }

    /* Get Weather and Time Service for this Location */
    std::unique_ptr<GeoWeatherServiceSIM> wService =
        std::make_unique<GeoWeatherServiceSIM>(std::to_string(gLoc.Latitude),
                                               std::to_string(gLoc.Longitude));

    if (wService->Initialize() != 0)
    {
      std::cerr << "Failed to Initialize Geo Weather Service for " << addr << std::endl;
      continue;
    }
    auto weather = wService->GetServiceData();
    if (!weather)
    {
      std::cout << "Invalid Geo Weather Data for " << addr << std::endl;
      continue;
    }

    PrintISensorData(weather);

    /* Get Date and Time Service for this Location */
    std::unique_ptr<GeoDateTimeServiceSIM>
        dtService = std::make_unique<GeoDateTimeServiceSIM>(wService->GetDateTimeStr());
    if (dtService->Initialize() != 0)
    {
      std::cerr << "Failed to Initialize Geo Time Service for " << addr << std::endl;
      continue;
    }
    auto dateTime = dtService->GetServiceData();
    if (!dateTime)
    {
      std::cout << "Invalid Geo Time Data for " << addr << std::endl;
      continue;
    }
    PrintISensorData(dateTime);

    /* Get Solar Radiation Data */
    std::unique_ptr<GeoSolarRadiationServiceSIM> solarService =
        std::make_unique<GeoSolarRadiationServiceSIM>(std::to_string(gLoc.Latitude),
                                                      std::to_string(gLoc.Longitude));

    if (solarService->Initialize() == 0)
    {
      auto solarData = solarService->GetServiceData();
      if (solarData)
        PrintISensorData(solarData);
      else
        std::cout << "Invalid Solar Radiation Data for " << addr << std::endl;
    }
    else
    {
      std::cerr << "Failed to Initialize Geo Solar Radiation Service for " << addr << std::endl;
    }

    /* Retrieve Magnetic and Geodatic Information */
    InData in;
    in.decimalYear = dateTime.GetDecimalYear();
    in.pos = gLoc;
    auto gMag_data = getDeclinition(&in);
    if (gMag_data)
    {
      PrintISensorData(gMag_data);
    }

    /* Retrieve Sun Position Information */
    SPA_Input spa_in(dateTime,
                     gLoc,
                     weather);

    auto spa_data = getSunPosition(&spa_in);
    if (spa_data)
    {
      PrintISensorData(spa_data);
    }
  }
  std::cout << "**************************************************************************************************************" << std::endl;
  return 0;
}