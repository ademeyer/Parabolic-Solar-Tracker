#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

/**
 * @brief Monte Carlo sensitivity analysis results
 */
struct MonteCarlResult
{
  std::vector<double> annual_energy_yield; // Distribution of annual energy (MWh)
  double mean_energy_mwh;
  double std_dev_mwh;
  double confidence_95_lower;
  double confidence_95_upper;
  int num_samples;

  MonteCarlResult()
      : mean_energy_mwh(0),
        std_dev_mwh(0),
        confidence_95_lower(0),
        confidence_95_upper(0),
        num_samples(0) {}

  void Calculate()
  {
    if (annual_energy_yield.empty())
      return;

    // Mean
    mean_energy_mwh =
        std::accumulate(annual_energy_yield.begin(), annual_energy_yield.end(), 0.0) /
        annual_energy_yield.size();

    // Standard deviation
    double variance = 0;
    for (const auto &val : annual_energy_yield)
    {
      double diff = val - mean_energy_mwh;
      variance += diff * diff;
    }
    std_dev_mwh = std::sqrt(variance / annual_energy_yield.size());

    // 95% confidence interval (assume normal distribution)
    // CI = mean ± 1.96 * std_dev / sqrt(n)
    double margin_of_error = 1.96 * std_dev_mwh / std::sqrt(annual_energy_yield.size());
    confidence_95_lower = mean_energy_mwh - margin_of_error;
    confidence_95_upper = mean_energy_mwh + margin_of_error;

    num_samples = annual_energy_yield.size();
  }

  void PrintSummary() const
  {
    std::cout << "\n====== Monte Carlo Sensitivity Analysis ======\n"
              << "Number of Samples: " << num_samples << "\n"
              << "Mean Annual Energy: " << std::fixed << std::setprecision(2)
              << mean_energy_mwh << " MWh/year\n"
              << "Std Dev: " << std_dev_mwh << " MWh\n"
              << "95% Confidence Interval: [" << confidence_95_lower << ", "
              << confidence_95_upper << "] MWh/year\n"
              << "Coefficient of Variation: " << (std_dev_mwh / mean_energy_mwh * 100)
              << " %\n";
  }
};

/**
 * @brief Parameter sensitivity result for Tornado plot
 */
struct ParameterSensitivity
{
  std::string parameter_name;
  double base_case_output;
  double low_variation_output;  // Output when parameter at -
  double high_variation_output; // Output when parameter at +
  double tornado_width;         // |high - low|
  double tornado_percentage;    // tornado_width / base_case * 100

  ParameterSensitivity()
      : parameter_name(""),
        base_case_output(0),
        low_variation_output(0),
        high_variation_output(0),
        tornado_width(0),
        tornado_percentage(0) {}

  void CalculateTornado(double base)
  {
    base_case_output = base;
    tornado_width = std::abs(high_variation_output - low_variation_output);
    tornado_percentage = (base > 0) ? (tornado_width / base * 100) : 0;
  }
};

/**
 * @brief Sensitivity analysis using Sobol indices (simplified Monte Carlo)
 */
class SensitivityAnalyzer
{
public:
  /**
   * @brief Generate Monte Carlo samples for uncertainty analysis
   * Varies key parameters with specified uncertainty ranges
   */
  static MonteCarlResult RunMonteCarloAnalysis(
      int num_samples = 1000,
      double dish_diameter_uncertainty_percent = 10,
      double wind_speed_uncertainty_percent = 20,
      double emissivity_uncertainty_percent = 5,
      double reflectivity_uncertainty_percent = 5,
      double dni_uncertainty_percent = 5)
  {
    MonteCarlResult result;

    // Base case parameters
    double base_dish_diameter = 5.0; // m
    double base_wind_speed = 5.0;    // m/s
    double base_emissivity = 0.1;
    double base_reflectivity = 0.95;
    double base_dni = 900; // W/m²

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> normal_dist(0, 1);

    // Run Monte Carlo samples
    for (int i = 0; i < num_samples; ++i)
    {
      // Sample parameters (normal distribution)
      double dish_d = base_dish_diameter *
                      (1 + (dish_diameter_uncertainty_percent / 100) * normal_dist(gen));
      double wind = base_wind_speed *
                    (1 + (wind_speed_uncertainty_percent / 100) * normal_dist(gen));
      wind = std::max(2.0, std::min(20.0, wind)); // Clamp to valid range

      double emiss = base_emissivity +
                     (emissivity_uncertainty_percent / 100) * normal_dist(gen);
      emiss = std::max(0.01, std::min(0.99, emiss)); // Clamp

      double refl = base_reflectivity +
                    (reflectivity_uncertainty_percent / 100) * normal_dist(gen);
      refl = std::max(0.5, std::min(0.99, refl)); // Clamp

      double dni = base_dni * (1 + (dni_uncertainty_percent / 100) * normal_dist(gen));
      dni = std::max(0.0, dni);

      // Calculate annual energy (simplified model)
      double annual_energy = CalculateAnnualEnergy(dish_d, wind, emiss, refl, dni);
      result.annual_energy_yield.push_back(annual_energy);
    }

    result.Calculate();
    return result;
  }

  /**
   * @brief Generate Tornado plot for parameter sensitivity ranking
   */
  static std::vector<ParameterSensitivity> GenerateTornadoPlot(
      double base_case_dni = 900)
  {
    std::vector<ParameterSensitivity> sensitivities;

    // Base case
    double base_output = CalculateAnnualEnergy(5.0, 5.0, 0.1, 0.95, base_case_dni);

    // Dish diameter sensitivity (3-15 m range)
    {
      ParameterSensitivity ps;
      ps.parameter_name = "Dish Diameter";
      ps.low_variation_output =
          CalculateAnnualEnergy(3.0, 5.0, 0.1, 0.95, base_case_dni);
      ps.high_variation_output =
          CalculateAnnualEnergy(15.0, 5.0, 0.1, 0.95, base_case_dni);
      ps.CalculateTornado(base_output);
      sensitivities.push_back(ps);
    }

    // Wind speed sensitivity
    {
      ParameterSensitivity ps;
      ps.parameter_name = "Wind Speed";
      ps.low_variation_output =
          CalculateAnnualEnergy(5.0, 2.0, 0.1, 0.95, base_case_dni);
      ps.high_variation_output =
          CalculateAnnualEnergy(5.0, 20.0, 0.1, 0.95, base_case_dni);
      ps.CalculateTornado(base_output);
      sensitivities.push_back(ps);
    }

    // Emissivity sensitivity
    {
      ParameterSensitivity ps;
      ps.parameter_name = "Receiver Emissivity";
      ps.low_variation_output =
          CalculateAnnualEnergy(5.0, 5.0, 0.05, 0.95, base_case_dni);
      ps.high_variation_output =
          CalculateAnnualEnergy(5.0, 5.0, 0.15, 0.95, base_case_dni);
      ps.CalculateTornado(base_output);
      sensitivities.push_back(ps);
    }

    // Reflectivity sensitivity
    {
      ParameterSensitivity ps;
      ps.parameter_name = "Mirror Reflectivity";
      ps.low_variation_output =
          CalculateAnnualEnergy(5.0, 5.0, 0.1, 0.90, base_case_dni);
      ps.high_variation_output =
          CalculateAnnualEnergy(5.0, 5.0, 0.1, 0.98, base_case_dni);
      ps.CalculateTornado(base_output);
      sensitivities.push_back(ps);
    }

    // DNI uncertainty
    {
      ParameterSensitivity ps;
      ps.parameter_name = "DNI (±5%)";
      ps.low_variation_output =
          CalculateAnnualEnergy(5.0, 5.0, 0.1, 0.95, base_case_dni * 0.95);
      ps.high_variation_output =
          CalculateAnnualEnergy(5.0, 5.0, 0.1, 0.95, base_case_dni * 1.05);
      ps.CalculateTornado(base_output);
      sensitivities.push_back(ps);
    }

    // Sort by tornado width (descending)
    std::sort(sensitivities.begin(), sensitivities.end(),
              [](const ParameterSensitivity &a, const ParameterSensitivity &b)
              { return a.tornado_percentage > b.tornado_percentage; });

    return sensitivities;
  }

  /**
   * @brief Plot Tornado diagram for sensitivity analysis
   */
  static void PlotTornadoDiagram(
      const std::vector<ParameterSensitivity> &sensitivities,
      const std::string &output_filename)
  {
    if (sensitivities.empty())
      return;

    plt::figure();

    std::vector<double> percentages;
    std::vector<std::string> names;

    for (const auto &s : sensitivities)
    {
      percentages.push_back(s.tornado_percentage);
      names.push_back(s.parameter_name);
    }

    // Create horizontal bar chart
    std::vector<double> y_pos(names.size());
    for (size_t i = 0; i < y_pos.size(); ++i)
      y_pos[i] = i;

    plt::barh(y_pos, percentages, 0.6, {{"color", "steelblue"}});
    plt::yticks(y_pos, names);
    plt::xlabel("Impact on Annual Energy (%)");
    plt::title("Parameter Sensitivity Analysis (Tornado Plot)");
    plt::grid(true, {{"axis", "x"}});

    plt::tight_layout();
    plt::save(output_filename + "_tornado_plot.png");
    plt::save(output_filename + "_tornado_plot.svg");
    plt::close();
  }

  /**
   * @brief Plot Monte Carlo results as histogram
   */
  static void PlotMonteCarloDistribution(
      const MonteCarlResult &result,
      const std::string &output_filename)
  {
    if (result.annual_energy_yield.empty())
      return;

    plt::figure();

    // Create histogram
    plt::hist(result.annual_energy_yield, 50, {{"edgecolor", "black"}});

    // Add mean and CI lines
    plt::axvline(result.mean_energy_mwh, {{"color", "red"}, {"linestyle", "-"}, {"label", "Mean"}});
    plt::axvline(result.confidence_95_lower,
                 {{"color", "orange"}, {"linestyle", "--"}, {"label", "95% CI"}});
    plt::axvline(result.confidence_95_upper,
                 {{"color", "orange"}, {"linestyle", "--"}});

    plt::xlabel("Annual Energy Yield (MWh/year)");
    plt::ylabel("Frequency");
    plt::title("Monte Carlo Results: Distribution of Annual Energy Yield");
    plt::legend();
    plt::grid(true);

    plt::tight_layout();
    plt::save(output_filename + "_monte_carlo_distribution.png");
    plt::save(output_filename + "_monte_carlo_distribution.svg");
    plt::close();
  }

private:
  /**
   * @brief Simplified annual energy calculation model
   * Used for sensitivity analysis
   */
  static double CalculateAnnualEnergy(
      double dish_diameter_m,
      double wind_speed_m_s,
      double emissivity,
      double reflectivity,
      double avg_dni_w_m2)
  {
    constexpr double DAYS_PER_YEAR = 365;
    constexpr double HOURS_PER_DAY = 8; // Average useful daylight hours

    // Aperture area
    double aperture_area = M_PI * std::pow(dish_diameter_m / 2, 2);

    // Incident power
    double incident_power_kw = (avg_dni_w_m2 * aperture_area) / 1000;

    // Optical efficiency (reflectivity-based)
    double optical_eff = reflectivity * 0.9; // 0.9 accounts for other losses

    // Thermal efficiency (wind and emissivity effects)
    double convection_factor = 1.0 - (wind_speed_m_s / 20.0) * 0.1; // Higher wind = more loss
    double radiation_factor = 1.0 - emissivity * 0.2;
    double thermal_eff = optical_eff * convection_factor * radiation_factor;

    // Annual energy
    double annual_energy_kwh = incident_power_kw * thermal_eff * HOURS_PER_DAY * DAYS_PER_YEAR;
    return annual_energy_kwh / 1000; // Convert to MWh
  }
};
