#pragma once
#include <map>
#include <regex>
#include "Collector.hpp"
#include "RayTraceVisualizer.hpp"
#include "SolarDatabaseManager.h"

class RunConfigParser : public ConfigParser<std::unordered_map<std::string, std::vector<std::string>>>
{
public:
  RunConfigParser(const std::string &filename = "run.conf") : ConfigParser<std::unordered_map<std::string, std::vector<std::string>>>(filename) {}
  const std::unordered_map<std::string, std::vector<std::string>> GetParsedData() const override
  {
    std::unordered_map<std::string, std::vector<std::string>> result;

    const std::regex pattern(R"([^",\s]+|"[^"]*")");

    for (const auto &mp : m_ParsedData)
    {
      std::vector<double> val;
      const auto &configs = mp.second;
      auto begin = std::sregex_iterator(configs.begin(), configs.end(), pattern);
      auto end = std::sregex_iterator();
      for (std::sregex_iterator it = begin; it != end; ++it)
        result[mp.first].push_back(it->str());
    }
    return result;
  }
};

class RunAnalysis
{
public:
  /* Data needed to run analysis */
  /**
   * Date: "2020-11-10"
   * Collector_configs: PDC-1: dish material name,reciever material name, dish diameter (m), dish depth (m) receiver diameter / width (m), receiver length [optional]
   * * Example: "Aluminum-Polished, Copper-Oxidized, 1.50, 1.1, 0.2" [Can add as may configs as needed]
   *
   */
  static void RunDailyAnalysis()
  {
    std::vector<std::pair<std::string, std::unique_ptr<Collector>>> collector_specs;
    std::string datestr;

    {
      auto runCong = std::make_unique<RunConfigParser>();
      if (!runCong->parseConfig())
      {
        std::cerr << "Failed to parse run.conf file\n";
        return;
      }

      auto configs = runCong->GetParsedData();
      if (configs.size() < 2)
      {
        std::cerr << "Configs is empty or incomplete\n";
        return;
      }

      // Configure collector specs
      const std::string prefix = "PDC-";
      auto cnt = 0;
      for (const auto &cfg : configs)
      {
        if (configs.find("Date") != configs.end() && datestr.empty())
        {
          if (configs["Date"].empty())
            continue;

          const auto &arr = configs["Date"];
          datestr = arr.at(0);
        }
        else
        {
          auto coll_name = prefix + std::to_string(++cnt);
          if (configs.find(coll_name) != configs.end())
          {
            const auto &c = cfg.second;
            if (c.size() < 5)
              continue;

            collector_specs.emplace_back(
                coll_name, std::make_unique<ParabolicDish>(c[0], c[1],
                                                           std::stod(c[2]), std::stod(c[3]),
                                                           std::stod(c[4]),
                                                           c.size() > 5 ? std::stod(c[5]) : 0.0));
          }
        }
      }
    }

    if (datestr.empty())
    {
      std::cerr << "No Date String found\n";
      return;
    }

    if (collector_specs.empty())
    {
      std::cerr << "No Collector Config found!\n";
      return;
    }

    // Retrieve data from DB
    using namespace dBCommon;
    std::unordered_map<std::string, DBLoggedData> dblogged_data;

    {
      auto sDB = std::make_unique<SolarDatabaseManager>();
      dblogged_data = sDB->GetDailyDBLoggedData(datestr);
    }

    if (dblogged_data.empty())
    {
      std::cerr << "No Log Found for " << datestr << std::endl;
      return;
    }

    // Begin Analysing
    for (auto &dbd : dblogged_data)
    {
      std::cout << "==================================================== Analysing Collector(s) in Location: " << dbd.first << " ====================================================" << std::endl;
      auto &dblog = dbd.second;
      auto &gLoc = dblog.m_Location;
      /* Analysing data between the 06:00 - 18:00 */
      std::map<std::string, std::vector<double>> efficiency;
      std::vector<std::string> time_interval;

      for (auto hr_start = 6; hr_start <= 18; hr_start += 3)
      {
        auto analysis_time = datestr + "T" + std::to_string(hr_start) + ":00:00TZ0";
        auto range_tsd = dblog.findTimeRange(analysis_time);
        if (range_tsd.empty())
        {
          std::cerr << "Range of time series data is empty for DateTime: " << analysis_time << std::endl;
          continue;
        }

        GeoSolarRadiationData avg_solar;
        GeoWeatherData avg_weather;
        // find Average data
        avg_solar = SolarRunninngAverage(range_tsd);
        avg_weather = WeatherRunninngAverage(range_tsd);

        // PrintISensorData(avg_solar);
        // PrintISensorData(avg_weather);

        if (!(avg_weather && avg_solar))
        {
          std::cerr << "Either or Both solar and weather data is Invalid\n";
          continue;
        }

        std::map<std::string, RayTraceResult> ray_results;
        for (const auto &col : collector_specs)
        {
          auto &c = col.second;
          if (!c->IsInitialized())
          {
            std::cerr << col.first << " is not initialized\n";
            continue;
          }

          {
            auto dateTime = GeoDateTimeData(analysis_time);
            std::cout
                << "==================== " << dbd.first << ": Running RayTrace Analysis for " << col.first << " =====================\n";

            auto result = c->RunAnalysis(dateTime, gLoc, avg_weather, avg_solar);
            ray_results[col.first] = result;

            auto pdc = dynamic_cast<ParabolicDish *>(c.get());
            auto receiverMaterial = pdc->GetReactorMaterial();
            efficiency[col.first].push_back(GetOpticalEfficiencyFromRayResult(result, receiverMaterial.absorptivity));
          }
        }
        std::cout << std::endl;
        RayPathVisualizer::Plot3DRayPaths(ray_results, (dbd.first + "_" + analysis_time));
        time_interval.push_back(std::to_string(hr_start));
      }
      ValueVisualizer::PlotValueOnBarGraph(efficiency, time_interval, (dbd.first + "_" + datestr));
    }
  }

private:
  static double GetOpticalEfficiencyFromRayResult(const RayTraceResult &ray_result,
                                                  const double &absorptivity = 1)
  {
    const int dish_id = 1, receiver_id = 2;
    int Nr = 0, Nd = 0;

    for (const auto &[id, flxmap] : ray_result.FluxMap)
    {
      if (id == dish_id)
        Nd = flxmap.size();
      else if (id == receiver_id)
        Nr = flxmap.size();
    }

    if (Nd == 0)
    {
      std::cerr << "Dish Ray Hits can not be zero\n";
      return 0.0;
    }
    return static_cast<double>((Nr / (double)Nd) * 100 * absorptivity);
  }

  static GeoSolarRadiationData SolarRunninngAverage(const std::vector<dBCommon::TimeSeriesData> &ts)
  {
    auto cur_avg = GeoSolarRadiationData(0.0, 0.0, 0.0, 0.0);
    long count = 0;
    for (const auto &i : ts)
    {
      cur_avg = cur_avg + i.p_SolarData;
      ++count;
    }
    return cur_avg / (double)count;
  }

  static GeoWeatherData WeatherRunninngAverage(const std::vector<dBCommon::TimeSeriesData> &ts)
  {
    auto cur_avg = GeoWeatherData(0.0, 0.0, 0.0, 0.0);
    long count = 0;
    for (const auto &i : ts)
    {
      cur_avg = cur_avg + i.p_WeatherData;
      ++count;
    }
    return cur_avg / count;
  }

  // General Print Helper Function
  template <typename T>
  static void PrintISensorData(const T &data)
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
};
