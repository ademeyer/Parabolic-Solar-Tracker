#pragma once
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "ThermalMetrics.hpp"

/**
 * @brief Results from a single simulation configuration
 */
struct SimulationResult
{
  std::string configuration_name;
  double dish_diameter_m;
  double focal_length_m;
  double receiver_diameter_m;
  double operating_temperature_celsius;
  double optical_efficiency_percent;
  double thermal_efficiency_percent;
  double annual_energy_mwh;
  std::string mirror_material;
  std::string receiver_coating;

  SimulationResult()
      : configuration_name(""),
        dish_diameter_m(0),
        focal_length_m(0),
        receiver_diameter_m(0),
        operating_temperature_celsius(0),
        optical_efficiency_percent(0),
        thermal_efficiency_percent(0),
        annual_energy_mwh(0),
        mirror_material(""),
        receiver_coating("") {}
};

/**
 * @brief Aggregates and compares simulation results across multiple configurations
 */
class ResultsComparator
{
public:
  /**
   * @brief Add a simulation result
   */
  void AddResult(const SimulationResult &result)
  {
    results.push_back(result);
  }

  /**
   * @brief Generate parametric optimization: Thermal efficiency vs. (Dish diameter, Focal length)
   * Creates a 3D response surface showing how thermal efficiency varies with geometry
   */
  struct ParametricOptimizationData
  {
    std::vector<double> dish_diameters;
    std::vector<double> focal_lengths;
    std::vector<std::vector<double>> thermal_efficiency_grid; // [dish_idx][focal_idx]
  };

  ParametricOptimizationData GenerateParametricOptimization() const
  {
    ParametricOptimizationData data;

    // Extract unique dish diameters and focal lengths
    std::set<double> unique_diameters, unique_focal_lengths;
    for (const auto &r : results)
    {
      unique_diameters.insert(r.dish_diameter_m);
      unique_focal_lengths.insert(r.focal_length_m);
    }

    data.dish_diameters.assign(unique_diameters.begin(), unique_diameters.end());
    data.focal_lengths.assign(unique_focal_lengths.begin(), unique_focal_lengths.end());

    // Sort for consistent ordering
    std::sort(data.dish_diameters.begin(), data.dish_diameters.end());
    std::sort(data.focal_lengths.begin(), data.focal_lengths.end());

    // Create grid
    data.thermal_efficiency_grid.resize(
        data.dish_diameters.size(),
        std::vector<double>(data.focal_lengths.size(), 0.0));

    // Fill grid
    for (size_t i = 0; i < data.dish_diameters.size(); ++i)
    {
      for (size_t j = 0; j < data.focal_lengths.size(); ++j)
      {
        double dish_d = data.dish_diameters[i];
        double focal_l = data.focal_lengths[j];

        // Find matching result
        for (const auto &r : results)
        {
          if (std::abs(r.dish_diameter_m - dish_d) < 0.01 &&
              std::abs(r.focal_length_m - focal_l) < 0.01)
          {
            data.thermal_efficiency_grid[i][j] = r.thermal_efficiency_percent;
            break;
          }
        }
      }
    }

    return data;
  }

  /**
   * @brief Generate material efficiency comparison
   */
  struct MaterialComparison
  {
    std::vector<std::string> material_combinations;
    std::vector<double> overall_efficiency;
  };

  MaterialComparison CompareMaterials() const
  {
    MaterialComparison comp;
    std::map<std::string, std::vector<double>> mat_efficiencies;

    for (const auto &r : results)
    {
      std::string combo = r.mirror_material + " + " + r.receiver_coating;
      mat_efficiencies[combo].push_back(r.thermal_efficiency_percent);
    }

    for (const auto &[combo, effs] : mat_efficiencies)
    {
      comp.material_combinations.push_back(combo);
      double avg_eff = std::accumulate(effs.begin(), effs.end(), 0.0) / effs.size();
      comp.overall_efficiency.push_back(avg_eff);
    }

    return comp;
  }

  /**
   * @brief Generate annual energy comparison
   */
  struct AnnualEnergyComparison
  {
    std::vector<std::string> configurations;
    std::vector<double> annual_energy_mwh;
    std::vector<double> percentage_gains; // vs baseline (first config)
  };

  AnnualEnergyComparison CompareAnnualEnergy() const
  {
    AnnualEnergyComparison comp;

    for (const auto &r : results)
    {
      comp.configurations.push_back(r.configuration_name);
      comp.annual_energy_mwh.push_back(r.annual_energy_mwh);
    }

    if (!comp.annual_energy_mwh.empty())
    {
      double baseline = comp.annual_energy_mwh[0];
      for (const auto &energy : comp.annual_energy_mwh)
      {
        double gain = (baseline > 0) ? ((energy - baseline) / baseline * 100) : 0;
        comp.percentage_gains.push_back(gain);
      }
    }

    return comp;
  }

  /**
   * @brief Export results to CSV file
   */
  void ExportToCSV(const std::string &filename) const
  {
    std::ofstream file(filename);
    if (!file.is_open())
    {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return;
    }

    // Header
    file << "Configuration,Dish_Diameter_m,Focal_Length_m,Receiver_Diameter_m,"
         << "Operating_Temp_C,Optical_Efficiency_%,Thermal_Efficiency_%,"
         << "Annual_Energy_MWh,Mirror_Material,Receiver_Coating\n";

    // Data
    for (const auto &r : results)
    {
      file << r.configuration_name << ","
           << std::fixed << std::setprecision(2)
           << r.dish_diameter_m << ","
           << r.focal_length_m << ","
           << r.receiver_diameter_m << ","
           << r.operating_temperature_celsius << ","
           << r.optical_efficiency_percent << ","
           << r.thermal_efficiency_percent << ","
           << r.annual_energy_mwh << ","
           << r.mirror_material << ","
           << r.receiver_coating << "\n";
    }

    file.close();
    std::cout << "Results exported to: " << filename << std::endl;
  }

  /**
   * @brief Get summary statistics
   */
  struct SummaryStats
  {
    double avg_optical_efficiency;
    double avg_thermal_efficiency;
    double max_annual_energy;
    double avg_operating_temperature;
    std::string best_configuration;
  };

  SummaryStats GetSummaryStatistics() const
  {
    SummaryStats stats;
    if (results.empty())
      return stats;

    std::vector<double> opt_eff, therm_eff, annual_energy, op_temp;
    for (const auto &r : results)
    {
      opt_eff.push_back(r.optical_efficiency_percent);
      therm_eff.push_back(r.thermal_efficiency_percent);
      annual_energy.push_back(r.annual_energy_mwh);
      op_temp.push_back(r.operating_temperature_celsius);
    }

    stats.avg_optical_efficiency =
        std::accumulate(opt_eff.begin(), opt_eff.end(), 0.0) / opt_eff.size();
    stats.avg_thermal_efficiency =
        std::accumulate(therm_eff.begin(), therm_eff.end(), 0.0) / therm_eff.size();
    stats.avg_operating_temperature =
        std::accumulate(op_temp.begin(), op_temp.end(), 0.0) / op_temp.size();

    auto max_it = std::max_element(annual_energy.begin(), annual_energy.end());
    stats.max_annual_energy = *max_it;
    stats.best_configuration =
        results[std::distance(annual_energy.begin(), max_it)].configuration_name;

    return stats;
  }

  void PrintSummary() const
  {
    auto stats = GetSummaryStatistics();
    std::cout << "\n====== Results Summary ======\n"
              << "Total Configs: " << results.size() << "\n"
              << "Average Optical Efficiency: " << std::fixed << std::setprecision(2)
              << stats.avg_optical_efficiency << " %\n"
              << "Average Thermal Efficiency: " << stats.avg_thermal_efficiency << " %\n"
              << "Average Operating Temperature: " << stats.avg_operating_temperature << " °C\n"
              << "Best Configuration: " << stats.best_configuration << "\n"
              << "Max Annual Energy: " << stats.max_annual_energy << " MWh/year\n";
  }

private:
  std::vector<SimulationResult> results;
};
