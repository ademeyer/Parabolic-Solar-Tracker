#pragma once
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

        auto coll_name = prefix + std::to_string(++cnt);
        if (configs.find(coll_name) != configs.end())
        {
          const auto &c = cfg.second;
          if (c.size() < 5)
            continue;

          collector_specs.emplace_back(
              coll_name, std::make_unique<Collector>(c[0], c[1],
                                                     std::stod(c[2]), std::stod(c[3]),
                                                     std::stod(c[4]),
                                                     c.size() > 5 ? std::stod(c[5]) : 0.0));
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
      std::cout << "Analysing Collector(s) in Location: " << dbd.first << std::endl;
      auto &dblog = dbd.second;
      auto &gLoc = dblog.m_Location;

      auto hr_start = 9;
      for (auto hr_start = 9; hr_start <= 18; hr_start += 3)
      {
        auto analysis_time = datestr + "T" + std::to_string(hr_start) + ":00:00";
        auto range_tsd = dblog.findTimeRange(analysis_time);
        if (range_tsd.empty())
        {
          std::cerr << "Range of time series data is empty for DateTime: " << analysis_time << std::endl;
          continue;
        }

        GeoSolarRadiationData avg_solar;
        GeoWeatherData avg_weather;
        // find Average data
        for (const auto &t : range_tsd)
        {
          avg_solar = SolarRunninngAverage(t.p_SolarData);
          avg_weather = WeatherRunninngAverage(t.p_WeatherData);
        }

        if (!(avg_weather.IsValid() && avg_solar.IsValid()))
        {
          std::cerr << "Either or Both solar and weather data is Invalid\n";
          continue;
        }

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
                << "==================== Running RayTrace Analysis for " << col.first << " =====================\n";
            auto results = c->RunAnalysis(dateTime, gLoc, avg_weather, avg_solar);

            RayPathVisualizer::Plot3DRayPaths(results, (col.first + "_" + analysis_time));
          }
        }
        std::cout << std::endl;
      }
    }
  }

private:
  static GeoSolarRadiationData SolarRunninngAverage(GeoSolarRadiationData new_val)
  {
    static auto cur_avg = GeoSolarRadiationData(0.0, 0.0, 0.0, 0.0);
    static long long count = 0;

    cur_avg = cur_avg + (static_cast<GeoSolarRadiationData>((new_val - cur_avg) / (double)count));

    ++count;

    return cur_avg;
  }

  static GeoWeatherData WeatherRunninngAverage(GeoWeatherData new_val)
  {
    static auto cur_avg = GeoWeatherData(0.0, 0.0, 0.0, 0.0);
    static long long count = 0;

    cur_avg = cur_avg + (static_cast<GeoWeatherData>((new_val - cur_avg) / (double)count));

    ++count;

    return cur_avg;
  }
};
