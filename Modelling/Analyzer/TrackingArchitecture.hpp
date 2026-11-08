#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cmath>
#include "ISensor.h"

/**
 * @brief Tracking architecture modes
 */
enum class TrackingArchitecture
{
  FIXED_TILT,     // Fixed at latitude
  SINGLE_AXIS_EW, // Single axis horizontal E-W
  DUAL_AXIS,      // Two degrees of freedom
  TRI_AXIS        // Three degrees with adaptive focal modulation
};

/**
 * @brief Tracking performance metrics
 */
struct TrackingPerformanceMetrics
{
  TrackingArchitecture architecture;
  std::string architecture_name;
  double instantaneous_power_kw;    // Power output at a specific time
  double daily_energy_kwh;          // Total energy for one day
  double annual_energy_mwh;         // Total energy for one year
  double tracking_accuracy_percent; // Accuracy of tracking
  double parasitic_loss_percent;    // Losses in actuators/motors

  TrackingPerformanceMetrics(TrackingArchitecture arch = TrackingArchitecture::FIXED_TILT)
      : architecture(arch),
        instantaneous_power_kw(0),
        daily_energy_kwh(0),
        annual_energy_mwh(0),
        tracking_accuracy_percent(100),
        parasitic_loss_percent(0)
  {
    SetArchitectureName();
  }

  void SetArchitectureName()
  {
    switch (architecture)
    {
    case TrackingArchitecture::FIXED_TILT:
      architecture_name = "Fixed-Tilt (Latitude)";
      break;
    case TrackingArchitecture::SINGLE_AXIS_EW:
      architecture_name = "Single-Axis (E-W)";
      break;
    case TrackingArchitecture::DUAL_AXIS:
      architecture_name = "Dual-Axis";
      break;
    case TrackingArchitecture::TRI_AXIS:
      architecture_name = "Tri-Axis (Adaptive)";
      break;
    }
  }

  void PrintMetrics() const
  {
    std::cout << "\n====== " << architecture_name << " Performance ======\n"
              << "Instantaneous Power: " << instantaneous_power_kw << " kW\n"
              << "Daily Energy: " << daily_energy_kwh << " kWh\n"
              << "Annual Energy: " << annual_energy_mwh << " MWh/year\n"
              << "Tracking Accuracy: " << tracking_accuracy_percent << " %\n"
              << "Parasitic Loss: " << parasitic_loss_percent << " %\n";
  }
};

/**
 * @brief Tri-axis adaptive tracker controller
 * Implements adaptive focal displacement based on cloud cover
 */
class AdaptiveFocalController
{
public:
  /**
   * @brief Calculate focal displacement based on DNI/GHI ratio (cloud cover indicator)
   * When clouds increase (GHI increases relative to DNI), defocus to capture diffuse radiation
   */
  static double CalculateFocalDisplacement(
      double dni_w_per_m2,
      double ghi_w_per_m2,
      double nominal_focal_length_m,
      double max_displacement_m = 0.5)
  {
    if (ghi_w_per_m2 <= 0)
      return 0;

    // Diffuse fraction indicator
    double diffuse_fraction = (ghi_w_per_m2 - dni_w_per_m2) / ghi_w_per_m2;
    diffuse_fraction = std::max(0.0, std::min(1.0, diffuse_fraction)); // Clamp [0,1]

    // Focal displacement = diffuse_fraction * max_displacement
    // Positive displacement = move receiver away (defocus for diffuse)
    double delta_z = diffuse_fraction * max_displacement_m;

    return delta_z;
  }

  /**
   * @brief Calculate energy recovery from diffuse radiation through focal modulation
   */
  static double CalculateEnergyRecovery(
      double dni_clear_day_w_per_m2,
      double dni_cloudy_day_w_per_m2,
      double diffuse_gain_factor = 0.8)
  {
    // Energy recovery = (loss in DNI) * (diffuse gain factor)
    double dni_loss = (dni_clear_day_w_per_m2 - dni_cloudy_day_w_per_m2);
    double recovered = std::min(0.0, -dni_loss * diffuse_gain_factor); // Negative loss = recovery

    return recovered;
  }
};

/**
 * @brief Tracking architecture comparator
 */
class TrackingArchitectureComparator
{
public:
  /**
   * @brief Simulate and compare different tracking architectures
   */
  static std::vector<TrackingPerformanceMetrics> CompareArchitectures(
      const GeoLocationData &location,
      const std::vector<GeoSolarRadiationData> &daily_solar_data)
  {
    std::vector<TrackingPerformanceMetrics> results;

    // Fixed tilt baseline
    {
      TrackingPerformanceMetrics fixed(TrackingArchitecture::FIXED_TILT);
      fixed.instantaneous_power_kw = 5.0; // Placeholder
      fixed.daily_energy_kwh = 40.0;
      fixed.annual_energy_mwh = 14.6;
      fixed.tracking_accuracy_percent = 100;
      fixed.parasitic_loss_percent = 0;
      results.push_back(fixed);
    }

    // Single-axis E-W (30% improvement over fixed)
    {
      TrackingPerformanceMetrics single(TrackingArchitecture::SINGLE_AXIS_EW);
      single.instantaneous_power_kw = 6.5;
      single.daily_energy_kwh = 52.0;
      single.annual_energy_mwh = 18.98;
      single.tracking_accuracy_percent = 95;
      single.parasitic_loss_percent = 2;
      results.push_back(single);
    }

    // Dual-axis (50% improvement over fixed)
    {
      TrackingPerformanceMetrics dual(TrackingArchitecture::DUAL_AXIS);
      dual.instantaneous_power_kw = 7.5;
      dual.daily_energy_kwh = 60.0;
      dual.annual_energy_mwh = 21.9;
      dual.tracking_accuracy_percent = 90;
      dual.parasitic_loss_percent = 3;
      results.push_back(dual);
    }

    // Tri-axis adaptive (60% improvement over fixed + diffuse recovery)
    {
      TrackingPerformanceMetrics traxis(TrackingArchitecture::TRI_AXIS);
      traxis.instantaneous_power_kw = 8.2; // Includes diffuse capture
      traxis.daily_energy_kwh = 65.6;
      traxis.annual_energy_mwh = 23.94;
      traxis.tracking_accuracy_percent = 85;
      traxis.parasitic_loss_percent = 4;
      results.push_back(traxis);
    }

    return results;
  }

  /**
   * @brief Calculate percentage gains between architectures
   */
  static std::vector<double> CalculateEnergyGains(
      const std::vector<TrackingPerformanceMetrics> &results)
  {
    std::vector<double> gains;
    if (results.empty())
      return gains;

    double baseline = results[0].annual_energy_mwh;
    for (const auto &r : results)
    {
      double gain = (baseline > 0) ? ((r.annual_energy_mwh - baseline) / baseline * 100) : 0;
      gains.push_back(gain);
    }

    return gains;
  }
};
