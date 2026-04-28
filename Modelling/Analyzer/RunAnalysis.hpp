#pragma once
#include <map>
#include <algorithm>
#include <iostream>
#include <ranges>
#include <regex>
#include "Collector.hpp"
#include "RayTraceVisualizer.hpp"
#include "SolarDatabaseManager.h"
#include "ThermalMetrics.hpp"

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

class AnalysisRunner
{
public:
  /* Data needed to run analysis */
  /**
   * Date: "2020-11-10"
   * Collector_configs: PDC-1: dish material name,reciever material name, dish diameter (m), dish depth (m) receiver diameter / width (m), receiver length [optional]
   * * Example: "Aluminum-Polished, Copper-Oxidized, 1.50, 1.1, 0.2" [Can add as may configs as needed]
   *
   */
  static void RunAnalysis()
  {
    std::vector<std::pair<std::string, std::unique_ptr<Collector>>> collector_specs;
    std::string sdatestr = "", edatestr = "", location = "";

    {
      auto runCong = std::make_unique<RunConfigParser>();
      if (!runCong->parseConfig())
      {
        std::cerr << "Failed to parse run.conf file\n";
        return;
      }

      auto configs = runCong->GetParsedData();
      if (configs.size() < 3)
      {
        std::cerr << "Configs is empty or incomplete\n";
        return;
      }

      // Configure collector specs
      const std::string prefix = "PDC-";
      auto cnt = 0;
      for (const auto &cfg : configs)
      {
        if (configs.find("S_Date") != configs.end() && sdatestr.empty())
        {
          if (configs["S_Date"].empty())
            continue;

          const auto &arr = configs["S_Date"];
          sdatestr = arr.at(0);
        }
        else if (configs.find("E_Date") != configs.end() && edatestr.empty())
        {
          if (configs["E_Date"].empty())
            continue;

          const auto &arr = configs["E_Date"];
          edatestr = arr.at(0);
        }
        else if (configs.find("Location") != configs.end() && location.empty())
        {
          if (configs["Location"].empty())
            continue;

          const auto &arr = configs["Location"];
          location = arr.at(0);
        }
        else
        {
          auto coll_name = prefix + std::to_string(++cnt);
          if (configs.find(coll_name) != configs.end())
          {
            const auto &c = cfg.second;
            if (c.size() < 6)
            {
              std::cerr << "Minimum argument expected to make a PDC object not met, " << c.size() << std::endl;
              continue;
            }

            collector_specs.emplace_back(
                coll_name, std::make_unique<ParabolicDish>(c[0], c[1], c[2],
                                                           std::stod(c[3]), std::stod(c[4]),
                                                           std::stod(c[5]),
                                                           c.size() > 6 ? std::stod(c[6]) : 0.0));
          }
        }
      }
    }

    if (location.empty())
    {
      std::cerr << "No location of interest selected, all location in the DB will be analysed\n";
    }

    if (sdatestr.empty() || edatestr.empty())
    {
      std::cerr << "No Date String found; [" << sdatestr << ", " << edatestr << "]\n";
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
      dblogged_data = location.empty() ? sDB->GetDBLoggedData(sdatestr, edatestr) : sDB->GetDBLoggedData(sdatestr, edatestr, location);
    }

    if (dblogged_data.empty())
    {
      std::cerr << "No Log Found for " << sdatestr << std::endl;
      return;
    }

    std::cout << "Size of retrieved logged data for " << dblogged_data.size() << " locations\n";
    for (const auto &[loc, data] : dblogged_data)
      std::cout << "Location: " << loc << ", Time Series Data Count: " << data.p_TimeSeriesData.size() << std::endl;

    /** Objective-1
     * To construct a comprehensive computational model combining optical
     * (ray tracing), thermal (conduction/convection/radiation),
     * sun positioning (SPA), tracking control, and IoT sensor simulation
     * modules into a unified, extensible architecture.
     */

    /** Objective-2
     * To conduct dynamic simulations comparing energy capture across
     * four tracking architectures (fixed-orientation, single-axis,
     * dual-axis, and tri-axis) under realistic diurnal and seasonal
     * irradiance variations.
     */

    /** Objective-3
     * To conduct parametric simulations systematically varying dish diameter
     * (3–15 m), focal length (1–6 m), combined with different material selections,
     * and receiver configurations to generate optical efficiency vs. concentration
     * ratio and thermal efficiency vs. operating temperature performance surfaces.
     */

    /** Objective-4
     * To compare simulated optical efficiency and thermal losses against theoretical
     * predictions and published experimental data from peer-reviewed literature.
     */

    /** Objective-5
     * To compare simulated optical efficiency and thermal losses against theoretical
     * predictions and published experimental data from peer-reviewed literature.
     */

    // Create Collector With Tracking for each collector spec
    const int used_pdc_idx = 0; // We only need to create tracked version for one PDC spec as the goal is to
                                // compare the same PDC with different tracking architecture, we can select any PDC spec for this purpose
    const std::vector<TrackingArchitecture> tracker_mode = {TrackingArchitecture::FIXED_TILT,
                                                            TrackingArchitecture::SINGLE_AXIS_EW,
                                                            TrackingArchitecture::DUAL_AXIS,
                                                            TrackingArchitecture::TRI_AXIS};
    // Recreate collector for tracking
    std::vector<std::pair<std::string, std::unique_ptr<CollectorWithTracking>>>
        collectorWithTracking_specs;

    /* We need to decorate PDC-1 only or another PDC, the goal is to maintain the same PDC properties with different tracking */
    const auto &pdc = collector_specs[used_pdc_idx].second;

    auto p = dynamic_cast<ParabolicDish *>(pdc.get());
    if (!p)
    {
      std::cerr << "dynamic casting failed, error!\n";
      throw std::runtime_error("dynamic casting failed, error!\n");
    }

    for (const auto &[track, colspecs] : std::views::zip(tracker_mode, collector_specs))
      collectorWithTracking_specs.emplace_back((colspecs.first + "_TR"), // Marked tracked
                                               std::make_unique<CollectorWithTracking>(p->Clone(), track));

    // Local-general parameters for analysis
    const int start_hr = 9; // 0-23
    const int end_hr = 18;  // 0-23
    const int hr_step = 3;  // any int
    constexpr int hour_cnt = std::min((24 / (((end_hr - start_hr) / hr_step) + 1)), 24);
    if (hour_cnt <= 0)
      throw std::range_error("Analysis time can not be less than zero");

    // Begin Analysing
    for (auto &dbd : dblogged_data)
    {
      std::cout << "==================================================== Analysing Collector(s) in Location: " << dbd.first << " ====================================================\n"
                << std::endl;
      auto &dblog = dbd.second;
      auto &gLoc = dblog.m_Location;

      auto start_d = DateTime(sdatestr + "T00:00:00TZ0").date;
      auto end_d = DateTime(edatestr + "T00:00:00TZ0").date;
      auto days_diff = end_d - start_d;

      std::map<std::string, std::vector<double>> daily_optical_efficiency; // pdc, daily average optical efficiency
      std::map<std::string, std::vector<double>> daily_thermal_efficiency; // pdc, daily average thermal efficiency
      std::map<std::string, std::vector<double>> daily_operating_temperature;
      std::map<std::string, std::vector<double>> daily_useful_energy;
      // Annual Yield (MWh/year)
      std::map<std::string, double> annual_yield; // pdc_energy : map [pdc_name:string, total_energy:double] per each location
      std::vector<std::string> day_interval;

      while (start_d <= end_d)
      {
        /* Analysing data between the 06:00 - 18:00 */
        std::map<std::string, std::vector<double>> efficiency;
        std::map<std::string, std::vector<double>> thermal_efficiency;
        std::map<std::string, std::vector<double>> operating_temperature;
        std::map<std::string, std::vector<double>> useful_energy;
        std::vector<std::string> time_interval;

        std::string analysis_time = "";

        for (auto hr_start = start_hr; hr_start <= end_hr; hr_start += hr_step)
        {
          analysis_time = std::string(start_d.c_str()) + "T" + std::to_string(hr_start) + ":00:00TZ0";
          auto range_tsd = dblog.findTimeRange(analysis_time);
          if (range_tsd.empty())
          {
            std::cerr << "Range of time series data is empty for DateTime: " << analysis_time << std::endl;
            continue;
          }

          GeoSolarRadiationData avg_solar;
          GeoWeatherData avg_weather;

          // Find Daily Average data
          avg_solar = SolarRunninngAverage(range_tsd);
          avg_weather = WeatherRunninngAverage(range_tsd);

          PrintISensorData(avg_solar);
          PrintISensorData(avg_weather);

          if (!(avg_weather && avg_solar))
          {
            std::cerr << "Either or Both solar and weather data is Invalid\n";
            continue;
          }

          std::map<std::string, RayTraceResult> ray_results;
          for (const auto &col : collector_specs)
          {
            auto &c = col.second;

            c->UpdateCollectorOrigin(gLoc);

            if (!c->IsInitialized())
            {
              std::cerr << col.first << " is not initialized\n";
              continue;
            }

            {
              auto dateTime = GeoDateTimeData(analysis_time);
              std::cout
                  << "==================== " << dbd.first << ": Running RayTrace Analysis for " << col.first << ", Time: " << analysis_time << " =====================\n";

              auto result = c->RunAnalysis(dateTime, gLoc, avg_weather, avg_solar);

              if (!result.FluxMap.empty())
              {

                ray_results[col.first] = result;

                double optical_eff = GetOpticalEfficiencyFromRayResult(result);
                efficiency[col.first].push_back(optical_eff);

                // Calculate thermal metrics
                ThermalResult thermal = CalculateThermalMetrics(result, *c, avg_weather, avg_solar);
                thermal.PrintMetrics();
                thermal_efficiency[col.first].push_back(thermal.thermal_efficiency);
                operating_temperature[col.first].push_back(thermal.operating_temperature_celsius);
                useful_energy[col.first].push_back(thermal.useful_energy_kw);
              }
              else
              {
                std::cerr << "Flux map is empty for collector: " << col.first << ", Time: " << analysis_time << std::endl;
                efficiency[col.first].push_back(0.0);
                thermal_efficiency[col.first].push_back(0.0);
                operating_temperature[col.first].push_back(0.0);
                useful_energy[col.first].push_back(0.0);
              }
            }
          }
          std::cout << std::endl;
          RayPathVisualizer::Plot3DRayPaths(ray_results, (dbd.first + "_" + analysis_time));
          time_interval.push_back(std::to_string(hr_start));

          // Run analysis for collectors with tracking and compare with fixed version to validate tracking
          // performance improvement, also compare with theoretical
          if (collectorWithTracking_specs.size() > used_pdc_idx)
          {
            std::cout << "==============================" << dbd.first << ": Running Annual Yield Comparison for fixed and tracked ==========================\n";
            // auto &annual_pdc = annual_yield[dbd.first];
            for (const auto &[pdc, c] : collectorWithTracking_specs)
            {
              c->UpdateCollectorOrigin(gLoc);
              if (c->IsInitialized())
              {
                std::cout << "==================== " << pdc << " is initialized and ready for analysis, Time: " << analysis_time << "====================" << std::endl;
                auto dateTime = GeoDateTimeData(analysis_time);
                auto result = c->RunAnalysis(dateTime, gLoc, avg_weather, avg_solar);

                if (result.FluxMap.empty())
                {
                  std::cerr << pdc << " returned an empty result in tracking mode\n";
                  annual_yield[pdc] += 0.0;
                  continue;
                }

                // extract thermal result
                ThermalResult thermal = CalculateThermalMetrics(result, *c, avg_weather, avg_solar);
                thermal.PrintMetrics();
                annual_yield[pdc] += thermal.incident_power_kw;
              }
              else // This was never expected to happen
              {
                std::cerr << pdc << " is not properly initialized!\n";
                break;
              }
            }
          }
        }

        // UpdateDailyAverageReadings(efficiency, daily_optical_efficiency);
        // UpdateDailyAverageReadings(thermal_efficiency, daily_thermal_efficiency);
        // UpdateDailyAverageReadings(operating_temperature, daily_operating_temperature);
        // UpdateDailyAverageReadings(useful_energy, daily_useful_energy);

        // day_interval.push_back(start_d.c_str());

        // ValueVisualizer::PlotValueOnBarGraph(efficiency, time_interval,
        //                                      (dbd.first + "_" + analysis_time + "_optical_efficiency"),
        //                                      "optical-efficiency-plots",
        //                                      "Efficiency (%)", "Time of Day",
        //                                      "Optical Efficiency Plot");
        // ValueVisualizer::PlotValueOnBarGraph(thermal_efficiency, time_interval,
        //                                      (dbd.first + "_" + analysis_time + "_thermal_efficiency"),
        //                                      "thermal-efficiency-plots",
        //                                      "Thermal Efficiency (%)", "Time of Day",
        //                                      "Thermal Efficiency Plot");
        // ValueVisualizer::PlotValueOnBarGraph(operating_temperature, time_interval,
        //                                      (dbd.first + "_" + analysis_time + "_operating_temperature"),
        //                                      "operating-temperature-plots",
        //                                      "Operating Temperature (°C)", "Time of Day",
        //                                      "Operating Temperature Plot");
        // ValueVisualizer::PlotValueOnBarGraph(useful_energy, time_interval,
        //                                      (dbd.first + "_" + analysis_time + "_useful_energy"),
        //                                      "useful-energy-plots",
        //                                      "Useful Energy (kW)", "Time of Day",
        //                                      "Useful Energy Plot");

        // // Also call dedicated thermal visualization functions
        // ValueVisualizer::PlotValueOnGraph(thermal_efficiency, time_interval,
        //                                   (dbd.first + "_" + analysis_time + "thermal_efficiency"),
        //                                   "thermal-efficiency-plots",
        //                                   "Thermal Efficiency (%)", "Time of Day",
        //                                   "Thermal Efficiency Plot");
        // ValueVisualizer::PlotValueOnGraph(operating_temperature, time_interval,
        //                                   (dbd.first + "_" + analysis_time + "operating_temperature"),
        //                                   "operating-temperature-plots",
        //                                   "Operating Temperature (°C)", "Time of Day",
        //                                   "Operating Temperature Plot");
        // ValueVisualizer::PlotValueOnGraph(useful_energy, time_interval,
        //                                   (dbd.first + "_" + analysis_time + "useful_energy"),
        //                                   "useful-energy-plots",
        //                                   "Useful Energy (kW)", "Time of Day",
        //                                   "Useful Energy Plot");
        ++start_d; // Next date
      }
      // PlotOpticalEfficiencyValidationOnBarGraph(daily_optical_efficiency, dbd.first);

      // ValueVisualizer::PlotValueOnGraph(daily_optical_efficiency, day_interval,
      //                                   (dbd.first + "_" + end_d.c_str() + "_daily_optical_efficiency"),
      //                                   "daily-plots",
      //                                   "Daily Average Optical Efficiency (%)", "Date",
      //                                   "Daily Average Optical Efficiency Plot");
      // ValueVisualizer::PlotValueOnGraph(daily_thermal_efficiency, day_interval,
      //                                   (dbd.first + "_" + end_d.c_str() + "_daily_thermal_efficiency"),
      //                                   "daily-plots",
      //                                   "Daily Average Thermal Efficiency (%)", "Date",
      //                                   "Daily Average Thermal Efficiency Plot");
      // ValueVisualizer::PlotValueOnGraph(daily_operating_temperature, day_interval,
      //                                   (dbd.first + "_" + end_d.c_str() + "_daily_operating_temperature"),
      //                                   "daily-plots",
      //                                   "Daily Average Operating Temperature (°C)", "Date",
      //                                   "Daily Average Operating Temperature Plot");
      // ValueVisualizer::PlotValueOnGraph(daily_useful_energy, day_interval,
      //                                   (dbd.first + "_" + end_d.c_str() + "_daily_useful_energy"),
      //                                   "daily-plots",
      //                                   "Daily Average Useful Energy (kW)", "Date",
      //                                   "Daily Average Useful Energy Plot");

      // Plot annual yield for this loc
      PlotAnnualEnergyComparison(annual_yield, days_diff, dbd.first);
    }

    // // 4.2.3	THERMAL MODEL VALIDATION AGAINST PUBLISHED DATA
    // {
    //   std::cout << "Start Validation.....\n";
    //   auto weather = GeoWeatherData(28, 1008.7, 88, 2.5);
    //   auto solar = GeoSolarRadiationData(800, 98.0, 162.0, 64.0);
    //   auto dateTime = GeoDateTimeData("2023-03-15T11:00:01TZ+03");
    //   auto location = GeoLocationData(32.021088, 44.402893, 34, 0.0);

    //   std::vector<std::pair<std::string, std::unique_ptr<Collector>>>
    //       specs;
    //   specs.emplace_back(std::string("PDC-0"), std::make_unique<ParabolicDish>("Aluminum-Polished", "Copper-Oxidized", "SilicaAerogel", 2, 0.301, 0.2));

    //   std::map<std::string, RayTraceResult> ray_results;

    //   // PrintISensorData(solar);
    //   // PrintISensorData(weather);
    //   // PrintISensorData(dateTime);
    //   // PrintISensorData(location);

    //   for (const auto &col : specs)
    //   {
    //     auto &c = col.second;
    //     c->UpdateCollectorOrigin(location);

    //     if (!c->IsInitialized())
    //     {
    //       std::cerr << col.first << " is not initialized\n";
    //       continue;
    //     }

    //     auto result = c->RunAnalysis(dateTime, location, weather, solar);
    //     if (result.FluxMap.empty())
    //     {
    //       std::cerr << "Flux Map is Empty for Validation";
    //       continue;
    //     }
    //     ray_results[col.first] = result;
    //     double optical_eff = GetOpticalEfficiencyFromRayResult(result);
    //     // std::cout << "optical efficiency: " << optical_eff << "%" << std::endl;
    //     ThermalResult thermal = CalculateThermalMetrics(result, *c, weather, solar);
    //     thermal.PrintMetrics();
    //   }
    // }
  }

private:
  static void PlotAnnualEnergyComparison(const std::map<std::string, double> &annuals, int days, const std::string &loc)
  {
    if (days <= 0)
      throw std::invalid_argument("Days should be higher than zero!");

    std::cout << "loc: " << loc << " size of annuals: " << annuals.size() << " days: " << days << std::endl;

    const std::vector<std::string> labels = {"Fixed Tilt", "Single-Axis EW", "Dual-Axis", "Tri-Axis"};
    std::map<std::string, std::vector<double>> plots; // pdc, total
    auto &p = plots[loc];

    for (const auto &[pdc, val] : annuals)
    {

      auto ret = (val * days) / 1000.0;
      // extrapolate for a year
      ret *= (365.0 / static_cast<double>(days));
      std::cout << pdc << ": " << ret << " MWh/year\n";
      p.push_back(ret); // assuming 9 - 18:00 -> MWh/year, appro.
    }

    ValueVisualizer::PlotValueOnBarGraph(plots, labels,
                                         (loc + "_Yearly_energy_comparison"),
                                         "energy-comparison-plots",
                                         "Energy (MWh/year)", "PDC Energy",
                                         "Annual Energy Comparison");
  }
  /**
   * @brief
   *
   * @param daily_optical_efficiency
   * @param loc
   */
  static void PlotOpticalEfficiencyValidationOnBarGraph(const std::map<std::string, std::vector<double>> &daily_optical_efficiency,
                                                        const std::string &loc)
  {
    std::map<std::string, std::vector<double>> cmp; // loc, pcd-1_opt, pcd-2_opt....
    auto &c = cmp[loc];
    std::vector<std::string> pdc_names;
    size_t n = INT_MAX;
    for (const auto &[pdc, daily_opt] : daily_optical_efficiency)
    {
      pdc_names.push_back(pdc);
      n = std::min(n, daily_opt.size());
    }

    size_t i = 0;
    while (i < n)
    {
      c.clear();
      for (const auto &pdc : pdc_names)
      {
        auto &v = daily_optical_efficiency.at(pdc);
        c.push_back(v[i]);
        // std::cerr << v[i] << ",";
      }
      // std::cerr << "\n";
      ValueVisualizer::PlotValueOnBarGraph(cmp, pdc_names,
                                           (loc + "_" + std::to_string(i) + "_optical_efficiency_at_normal_incidence"),
                                           "optical-efficiency-validation-plots",
                                           "Efficiency (%)", "PDC Config",
                                           "Optical Efficiency Validation");
      ++i;
    }
  }

  /**
   * @brief Calculate thermal metrics for a collector
   * @param ray_result Ray tracing result containing flux information
   * @param collector Collector object with material properties
   * @param weather Weather data (temperature, wind speed)
   * @param avg_solar Solar radiation data
   * @return ThermalResult containing all thermal metrics
   */
  static ThermalResult CalculateThermalMetrics(
      const RayTraceResult &ray_result,
      const Collector &collector,
      const GeoWeatherData &weather,
      const GeoSolarRadiationData &avg_solar)
  {
    ThermalResult thermal_result;

    // Step 1: Calculate incident solar power
    // Assuming a standard reference area (receiver area from ray results)
    const double opt_eff = GetOpticalEfficiencyFromRayResult(ray_result);
    const double receiver_area = collector.GetAbsorberArea();

    thermal_result.incident_power_kw = (avg_solar.DNI * collector.GetReflectorArea()) / 1000.0; // Convert to kW

    // Step 2: Calculate absorbed power from ray tracing result
    // optical efficiency already accounts for reflection losses
    auto absorber_mat = collector.GetAbsorberMaterial();
    thermal_result.absorbed_power_kw =
        thermal_result.incident_power_kw * opt_eff / 100.0;

    auto insulator_mat = collector.GetInsulatorMaterial();

    thermal_result.operating_temperature_celsius =
        ThermalCalculator::SolveReceiverTemperature(
            thermal_result.absorbed_power_kw * 1000.0,
            receiver_area,
            insulator_mat.thickness, weather.temp,
            absorber_mat.emissivity, weather.wind_speed,
            insulator_mat.thermal_conductivity, 112.0);

    // Step 4: Calculate radiation loss
    double delta_temp = thermal_result.operating_temperature_celsius - weather.temp;
    if (delta_temp > 0)
    {
      thermal_result.radiation_loss_kw =
          ThermalCalculator::CalculateRadiationLoss(
              absorber_mat.emissivity,
              receiver_area,
              thermal_result.operating_temperature_celsius,
              weather.temp) /
          1000.0; // Convert to kW
    }

    // Step 5: Calculate convection loss
    double wind_speed = weather.wind_speed > 0 ? weather.wind_speed : 2.0; // Default 2 m/s
    thermal_result.convection_loss_kw =
        ThermalCalculator::CalculateConvectionLoss(
            receiver_area,
            thermal_result.operating_temperature_celsius,
            weather.temp,
            wind_speed) /
        1000.0; // Convert to kW

    // Step 6: Calculate conduction loss (if material properties available)
    thermal_result.conduction_loss_kw = 0;
    if (delta_temp > 0)
    {
      auto insulator_mat = collector.GetInsulatorMaterial();
      thermal_result.conduction_loss_kw = ThermalCalculator::CalculateConductionLoss(
                                              insulator_mat.thermal_conductivity,
                                              receiver_area,
                                              thermal_result.operating_temperature_celsius,
                                              weather.temp,
                                              insulator_mat.thickness) /
                                          1000.00; // convert to kW
    }

    // Step 7: Calculate total losses and net useful energy
    thermal_result.total_loss_kw =
        thermal_result.radiation_loss_kw +
        thermal_result.convection_loss_kw +
        thermal_result.conduction_loss_kw;

    thermal_result.useful_energy_kw =
        std::max(0.0, thermal_result.absorbed_power_kw -
                          thermal_result.total_loss_kw);

    // Step 8: Calculate thermal efficiency
    if (thermal_result.incident_power_kw > 0)
    {
      thermal_result.thermal_efficiency =
          (thermal_result.useful_energy_kw / thermal_result.incident_power_kw) * 100;
    }

    // Step 9: Energy balance check
    double total_energy = thermal_result.useful_energy_kw + thermal_result.total_loss_kw;
    if (total_energy > 0)
    {
      thermal_result.energy_balance_error_percent =
          ((thermal_result.absorbed_power_kw - total_energy) /
           thermal_result.absorbed_power_kw) *
          100;
    }

    return thermal_result;
  }

  /**
   * @brief Get Daily efficiency for each collector
   *
   * @param eff
   * @return void
   */
  static void UpdateDailyAverageReadings(const std::map<std::string, std::vector<double>> &eff, std::map<std::string, std::vector<double>> &daily_eff)
  {
    for (const auto &[pdc, values] : eff)
    {
      double daily_avg = 0.0;
      int cnt = 0;
      for (const auto &v : values)
      {
        daily_avg += v;
        ++cnt;
      }
      daily_eff[pdc].push_back(daily_avg /= cnt);
    }
  }

  /**
   * @brief Get the Optical Efficiency From Ray Result object
   *
   * @param ray_result
   * @return double
   */
  static double GetOpticalEfficiencyFromRayResult(const RayTraceResult &ray_result)
  {
    const int receiver_id = 2;
    const auto &ray = ray_result.RayCount;

    if (ray == 0)
    {
      std::cerr << "Generated Rays can not be zero\n";
      return 0.0;
    }

    if (ray_result.FluxMap.count(receiver_id) == 0)
    {
      std::cerr << "Receiver stage " << receiver_id << " not found in flux map\n";
      return 0.0;
    }

    int Nr = ray_result.FluxMap.at(receiver_id).size();

    return static_cast<double>((Nr / (double)ray) * 100);
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
