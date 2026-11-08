#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <random>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

/**
 * @brief Optimization convergence metrics
 */
struct OptimizationConvergence
{
  std::string optimization_name;
  int num_iterations;
  std::vector<double> fitness_history; // Best fitness at each iteration
  std::vector<double> population_avg;  // Average fitness of population
  double final_best_fitness;
  double initial_best_fitness;
  int iterations_to_convergence;
  double convergence_percentage;

  OptimizationConvergence()
      : optimization_name(""),
        num_iterations(0),
        final_best_fitness(0),
        initial_best_fitness(0),
        iterations_to_convergence(0),
        convergence_percentage(0) {}

  void CalculateConvergenceMetrics()
  {
    if (fitness_history.empty())
      return;

    final_best_fitness = fitness_history.back();
    initial_best_fitness = fitness_history.front();

    // Find iteration where convergence occurs (99% of final value)
    double target = initial_best_fitness + 0.99 * (final_best_fitness - initial_best_fitness);
    for (size_t i = 0; i < fitness_history.size(); ++i)
    {
      if (fitness_history[i] >= target)
      {
        iterations_to_convergence = i;
        break;
      }
    }

    convergence_percentage = 100.0; // For this placeholder
  }

  void PrintMetrics() const
  {
    std::cout << "\n====== " << optimization_name << " Convergence ======\n"
              << "Initial Fitness: " << initial_best_fitness << "\n"
              << "Final Fitness: " << final_best_fitness << "\n"
              << "Improvement: " << (final_best_fitness - initial_best_fitness) << "\n"
              << "Iterations to 99% Convergence: " << iterations_to_convergence << "\n"
              << "Total Iterations: " << num_iterations << "\n";
  }
};

/**
 * @brief Annual energy aggregation results
 */
struct AnnualEnergyResults
{
  struct MonthlyData
  {
    std::string month_name;
    double daily_avg_kwh;
    double monthly_total_kwh;
    double percent_of_annual;
  };

  std::vector<MonthlyData> monthly_breakdown;
  double annual_total_mwh;
  double daily_average_kwh;
  double peak_day_kwh;
  double lowest_day_kwh;
  double std_dev_kwh;

  AnnualEnergyResults()
      : annual_total_mwh(0),
        daily_average_kwh(0),
        peak_day_kwh(0),
        lowest_day_kwh(0),
        std_dev_kwh(0) {}

  void PrintSummary() const
  {
    std::cout << "\n====== Annual Energy Summary ======\n"
              << "Annual Total: " << std::fixed << std::setprecision(2)
              << annual_total_mwh << " MWh\n"
              << "Daily Average: " << daily_average_kwh << " kWh\n"
              << "Peak Day: " << peak_day_kwh << " kWh\n"
              << "Lowest Day: " << lowest_day_kwh << " kWh\n"
              << "Std Dev: " << std_dev_kwh << " kWh\n";

    std::cout << "\nMonthly Breakdown:\n"
              << "-" << std::string(60, '-') << "\n";
    for (const auto &m : monthly_breakdown)
    {
      std::cout << m.month_name << ": " << m.monthly_total_kwh << " kWh ("
                << m.percent_of_annual << "%)\n";
    }
  }
};

/**
 * @brief Optimization and annual aggregation engine
 */
class OptimizationAndAggregationEngine
{
public:
  /**
   * @brief Generate synthetic GA convergence curve for visualization
   * In a production system, this would use real GA data
   */
  static OptimizationConvergence GenerateGAConvergence(int num_iterations = 100)
  {
    OptimizationConvergence conv;
    conv.optimization_name = "Genetic Algorithm (GA) - Parameter Optimization";
    conv.num_iterations = num_iterations;

    // Generate synthetic convergence curve
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> noise_dist(0, 1);

    double fitness = 0;
    for (int i = 0; i < num_iterations; ++i)
    {
      // Convergence curve: exponential approach to optimum
      double progress = 1.0 - std::exp(-3.0 * i / num_iterations);
      fitness = 100 + 150 * progress + (noise_dist(gen) - 0.5) * 5; // Fitness 100-250

      conv.fitness_history.push_back(fitness);

      // Population average (slightly lower than best)
      double pop_avg = fitness - 10 - (i / num_iterations) * 20;
      conv.population_avg.push_back(std::max(100.0, pop_avg));
    }

    conv.CalculateConvergenceMetrics();
    return conv;
  }

  /**
   * @brief Generate synthetic PID control response
   * Shows setpoint vs actual angle tracking over time
   */
  static std::pair<std::vector<double>, std::vector<double>> GeneratePIDControlResponse(
      int num_timesteps = 144) // 10 min intervals over 24 hours
  {
    std::vector<double> setpoint, actual;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> noise_dist(-2, 2);

    double error_integral = 0;
    double prev_error = 0;

    constexpr double kp = 1.0; // Proportional gain
    constexpr double ki = 0.1; // Integral gain
    constexpr double kd = 0.5; // Derivative gain

    for (int i = 0; i < num_timesteps; ++i)
    {
      // Simulated setpoint from SPA
      double sun_angle = -90 + 180 * (i / (double)num_timesteps);
      setpoint.push_back(sun_angle);

      // Simulated tracking with minor lag and noise
      double target = sun_angle - error_integral * ki - (error_integral - prev_error) * kd;
      double tracked = target + noise_dist(gen);

      error_integral += (sun_angle - tracked);
      prev_error = sun_angle - tracked;

      actual.push_back(tracked);
    }

    return {setpoint, actual};
  }

  /**
   * @brief Generate annual energy aggregation from daily data
   */
  static AnnualEnergyResults AggregateAnnualEnergy(
      const std::vector<double> &daily_energy_kwh)
  {
    AnnualEnergyResults results;

    if (daily_energy_kwh.empty())
      return results;

    // Annual total
    results.annual_total_mwh =
        std::accumulate(daily_energy_kwh.begin(), daily_energy_kwh.end(), 0.0) / 1000;

    // Daily average
    results.daily_average_kwh =
        results.annual_total_mwh * 1000 / daily_energy_kwh.size();

    // Peak and lowest
    auto minmax = std::minmax_element(daily_energy_kwh.begin(), daily_energy_kwh.end());
    results.lowest_day_kwh = *minmax.first;
    results.peak_day_kwh = *minmax.second;

    // Std dev
    double variance = 0;
    for (const auto &val : daily_energy_kwh)
    {
      double diff = val - results.daily_average_kwh;
      variance += diff * diff;
    }
    results.std_dev_kwh = std::sqrt(variance / daily_energy_kwh.size());

    // Monthly breakdown
    std::vector<std::string> months = {"January", "February", "March", "April", "May",
                                       "June", "July", "August", "September", "October",
                                       "November", "December"};
    std::vector<int> days_per_month = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    size_t day_idx = 0;
    for (size_t m = 0; m < months.size() && day_idx < daily_energy_kwh.size(); ++m)
    {
      AnnualEnergyResults::MonthlyData monthly;
      monthly.month_name = months[m];

      double month_total = 0;
      int days_counted = 0;
      for (int d = 0; d < days_per_month[m] && day_idx < daily_energy_kwh.size(); ++d)
      {
        month_total += daily_energy_kwh[day_idx++];
        days_counted++;
      }

      monthly.monthly_total_kwh = month_total;
      monthly.daily_avg_kwh = month_total / days_counted;
      monthly.percent_of_annual =
          (results.annual_total_mwh * 1000 > 0)
              ? (month_total / (results.annual_total_mwh * 1000) * 100)
              : 0;

      results.monthly_breakdown.push_back(monthly);
    }

    return results;
  }

  /**
   * @brief Plot GA convergence curve
   */
  static void PlotGAConvergence(
      const OptimizationConvergence &convergence,
      const std::string &output_filename)
  {
    if (convergence.fitness_history.empty())
      return;

    plt::figure();

    std::vector<double> iterations;
    for (size_t i = 0; i < convergence.fitness_history.size(); ++i)
      iterations.push_back(i);

    plt::plot(iterations, convergence.fitness_history,
              {{"label", "Best Fitness"}, {"marker", "o"}, {"markersize", 2}});
    plt::plot(iterations, convergence.population_avg,
              {{"label", "Population Average"}, {"linestyle", "--"}});

    plt::title("Genetic Algorithm: Fitness Convergence");
    plt::xlabel("Generation");
    plt::ylabel("Fitness Function Value");
    plt::legend();
    plt::grid(true);

    plt::tight_layout();
    plt::save(output_filename + "_ga_convergence.png");
    plt::save(output_filename + "_ga_convergence.svg");
    plt::close();
  }

  /**
   * @brief Plot PID control response
   */
  static void PlotPIDResponse(
      const std::pair<std::vector<double>, std::vector<double>> &response,
      const std::string &output_filename)
  {
    if (response.first.empty() || response.second.empty())
      return;

    plt::figure();

    std::vector<double> time_steps;
    for (size_t i = 0; i < response.first.size(); ++i)
      time_steps.push_back(i);

    plt::plot(time_steps, response.first,
              {{"label", "Setpoint (Optimal SPA)"}, {"marker", "o"}, {"markersize", 2}});
    plt::plot(time_steps, response.second,
              {{"label", "Actual Tracking Angle"}, {"marker", "s"}, {"markersize", 2}});

    plt::title("PID Tracking Control: Setpoint vs Actual Response");
    plt::xlabel("Time Step (10-min intervals)");
    plt::ylabel("Tracking Angle (degrees)");
    plt::legend();
    plt::grid(true);

    plt::tight_layout();
    plt::save(output_filename + "_pid_response.png");
    plt::save(output_filename + "_pid_response.svg");
    plt::close();
  }

  /**
   * @brief Plot annual energy aggregation
   */
  static void PlotAnnualEnergyAggregation(
      const AnnualEnergyResults &results,
      const std::string &output_filename)
  {
    if (results.monthly_breakdown.empty())
      return;

    plt::figure();

    std::vector<double> monthly_energy;
    std::vector<std::string> month_names;

    for (const auto &m : results.monthly_breakdown)
    {
      monthly_energy.push_back(m.monthly_total_kwh / 1000); // Convert to MWh
      month_names.push_back(m.month_name);
    }

    plt::bar(month_names, monthly_energy, 0.7, {{"color", "steelblue"}});
    plt::title("Annual Energy Yield by Month");
    plt::xlabel("Month");
    plt::ylabel("Energy (MWh)");
    plt::xticks(std::vector<double>(), month_names, {{"rotation", 45.0}});
    plt::grid(true, {{"axis", "y"}});

    // Add total annotation
    std::string total_text = "Total: " + std::to_string((int)results.annual_total_mwh) +
                             " MWh/year";
    plt::text(6, *std::max_element(monthly_energy.begin(), monthly_energy.end()) * 0.9,
              total_text);

    plt::tight_layout();
    plt::save(output_filename + "_annual_energy.png");
    plt::save(output_filename + "_annual_energy.svg");
    plt::close();
  }

  /**
   * @brief Plot tracking comparison over a representative day
   */
  static void PlotTrackingComparison(
      const std::vector<double> &time_hours,
      const std::vector<double> &fixed_tilt,
      const std::vector<double> &single_axis,
      const std::vector<double> &dual_axis,
      const std::vector<double> &tri_axis,
      const std::string &output_filename)
  {
    if (time_hours.empty())
      return;

    plt::figure();

    plt::plot(time_hours, fixed_tilt,
              {{"label", "Fixed-Tilt"}, {"marker", "o"}, {"markersize", 3}});
    plt::plot(time_hours, single_axis,
              {{"label", "Single-Axis (E-W)"}, {"marker", "s"}, {"markersize", 3}});
    plt::plot(time_hours, dual_axis,
              {{"label", "Dual-Axis"}, {"marker", "^"}, {"markersize", 3}});
    plt::plot(time_hours, tri_axis,
              {{"label", "Tri-Axis (Adaptive)"}, {"marker", "*"}, {"markersize", 8}});

    plt::title("Instantaneous Thermal Power Output: Tracking Architecture Comparison");
    plt::xlabel("Time of Day (hours)");
    plt::ylabel("Thermal Power Output (kW)");
    plt::legend();
    plt::grid(true);
    plt::xlim(6, 18);

    plt::tight_layout();
    plt::save(output_filename + "_tracking_comparison.png");
    plt::save(output_filename + "_tracking_comparison.svg");
    plt::close();
  }
};
