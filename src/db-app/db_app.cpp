#include "APIManager.hpp"
#include "SolarDatabaseManager.h"

int main()
{
  std::unique_ptr<APIManager> api = std::make_unique<APIManager>();
  int retry_times = 2;
  auto res = APIERROR::NOERROR;
  do
  {
    res = api->InitializeServices();

  } while (--retry_times >= 0 && res != APIERROR::NOERROR);

  if (res != APIERROR::NOERROR)
  {
    auto error_code = static_cast<int>(res);
    std::cout << "API Manager exit with Error: " << error_code << std::endl;
    return error_code;
  }

  auto data_values = api->GetServicesData();
  std::unique_ptr<SolarDatabaseManager> sDB = std::make_unique<SolarDatabaseManager>();

  for (const auto &data : data_values)
  {
    using namespace dBCommon;
    auto loc_id = -1;
    {
      DBLocationData loc_table;
      loc_table.locs = data.location;
      loc_table.name = data.loc_name;
      if (!sDB->Insert(loc_table))
      {
        std::cerr << "Location Table Update Failed" << std::endl;
        break;
      }
    }

    if ((loc_id = sDB->GetLocationIdWithName(data.loc_name)) == -1)
    {
      std::cerr << "Unable to get Location id\n";
      continue;
    }

    {
      DBWeatherData weather_table;
      weather_table.location_id = loc_id;
      weather_table.logTime = data.datetime.dt;
      weather_table.weatherData = data.weather;
      if (!sDB->Insert(weather_table))
      {
        std::cerr << "Weather Table Update Failed in location: " << data.loc_name << " loc_id: " << loc_id << std::endl;
      }
    }

    {
      DBSolarData solar_table;
      solar_table.location_id = loc_id;
      solar_table.logTime = data.datetime.dt;
      solar_table.srd = data.solar;
      if (!sDB->Insert(solar_table))
      {
        std::cerr << "Solar Table Update Failed in location: " << data.loc_name << " loc_id: " << loc_id << std::endl;
      }
    }
  }

  return 0;
}