#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

/**
 * @brief NREL SPA validation data structure
 */
struct SPAValidationResult
{
  std::string test_case_name;
  double timestamp;

  // Calculated values (from our SPA)
  double calc_zenith_angle_deg;
  double calc_azimuth_deg;
  double calc_elevation_angle_deg;
  double calc_incidence_angle_deg;

  // Reference values (NREL)
  double ref_zenith_angle_deg;
  double ref_azimuth_deg;
  double ref_elevation_angle_deg;
  double ref_incidence_angle_deg;

  // Differences
  double zenith_error_deg;
  double azimuth_error_deg;
  double elevation_error_deg;
  double incidence_error_deg;
  double max_error_deg;

  SPAValidationResult()
      : test_case_name(""),
        timestamp(0),
        calc_zenith_angle_deg(0),
        calc_azimuth_deg(0),
        calc_elevation_angle_deg(0),
        calc_incidence_angle_deg(0),
        ref_zenith_angle_deg(0),
        ref_azimuth_deg(0),
        ref_elevation_angle_deg(0),
        ref_incidence_angle_deg(0),
        zenith_error_deg(0),
        azimuth_error_deg(0),
        elevation_error_deg(0),
        incidence_error_deg(0),
        max_error_deg(0) {}

  void CalculateErrors()
  {
    zenith_error_deg = std::abs(calc_zenith_angle_deg - ref_zenith_angle_deg);
    azimuth_error_deg = std::abs(calc_azimuth_deg - ref_azimuth_deg);
    elevation_error_deg = std::abs(calc_elevation_angle_deg - ref_elevation_angle_deg);
    incidence_error_deg = std::abs(calc_incidence_angle_deg - ref_incidence_angle_deg);

    max_error_deg = std::max({zenith_error_deg, azimuth_error_deg,
                              elevation_error_deg, incidence_error_deg});
  }

  void PrintResult() const
  {
    std::cout << "\n====== SPA Validation: " << test_case_name << " ======\n"
              << "Zenith:    Calc=" << std::fixed << std::setprecision(4) << calc_zenith_angle_deg
              << "°, Ref=" << ref_zenith_angle_deg << "°, Error=" << zenith_error_deg << "°\n"
              << "Azimuth:   Calc=" << calc_azimuth_deg << "°, Ref=" << ref_azimuth_deg
              << "°, Error=" << azimuth_error_deg << "°\n"
              << "Elevation: Calc=" << calc_elevation_angle_deg << "°, Ref=" << ref_elevation_angle_deg
              << "°, Error=" << elevation_error_deg << "°\n"
              << "Incidence: Calc=" << calc_incidence_angle_deg << "°, Ref=" << ref_incidence_angle_deg
              << "°, Error=" << incidence_error_deg << "°\n"
              << "Max Error: " << max_error_deg << "°\n";
  }
};

/**
 * @brief SPA Validator class
 */
class SPAValidator
{
public:
  static void ValidateSPA(const std::vector<SPAValidationResult> &results,
                          const std::string &output_filename)
  {
    if (results.empty())
      return;

    // Create comparison plot
    plt::figure();

    std::vector<double> test_indices;
    std::vector<double> max_errors;
    std::vector<std::string> test_names;

    for (size_t i = 0; i < results.size(); ++i)
    {
      test_indices.push_back(i);
      max_errors.push_back(results[i].max_error_deg);
      test_names.push_back(results[i].test_case_name);
    }

    // Plot max errors
    plt::bar(test_indices, max_errors, 0.6, {{"color", "steelblue"}});

    // Add reference line at ±0.0003°
    std::vector<double> ref_line(test_indices.size(), 0.0003);
    plt::plot(test_indices, ref_line, {{"label", "NREL Target (±0.0003°)"}, {"linestyle", "--"}, {"color", "red"}});

    plt::title("SPA Validation: Max Angular Deviation vs NREL Benchmark");
    plt::xlabel("Test Case");
    plt::ylabel("Max Error (degrees)");
    plt::legend();
    plt::grid(true);

    plt::xticks(test_indices, test_names, {{"rotation", 45.0}});
    plt::tight_layout();
    plt::save(output_filename + "_spa_validation.png");
    plt::save(output_filename + "_spa_validation.svg");
    plt::close();

    // Print statistics
    double max_overall = *std::max_element(max_errors.begin(), max_errors.end());
    double avg_error = std::accumulate(max_errors.begin(), max_errors.end(), 0.0) /
                       max_errors.size();
    bool all_pass = std::all_of(max_errors.begin(), max_errors.end(),
                                [](double e)
                                { return e < 0.001; }); // ±0.001° threshold

    std::cout << "\n====== SPA Validation Summary ======\n"
              << "Total Test Cases: " << results.size() << "\n"
              << "Average Max Error: " << std::fixed << std::setprecision(6) << avg_error << "°\n"
              << "Maximum Error: " << max_overall << "°\n"
              << "NREL Target (±0.0003°): " << (all_pass ? "PASS" : "FAIL") << "\n";
  }
};

/**
 * @brief Incidence Angle Modifier (IAM) validation
 */
struct IAMValidationData
{
  std::vector<double> incidence_angle_deg;
  std::vector<double> simulated_iam;   // From ray tracing
  std::vector<double> theoretical_iam; // From model
  std::string collector_type;
};

/**
 * @brief IAM Validator class
 */
class IAMValidator
{
public:
  /**
   * @brief Calculate theoretical IAM using Souleiman-Kinghorn model
   * IAM(θ) = 1.0 - b0*(sec(θ) - 1) for flat plate
   * IAM(θ) = 1.0 - b0*tan(θ)^2 for concentrator
   */
  static double CalculateTheoreticalIAM(double incidence_angle_deg,
                                        double b0 = 0.0,
                                        bool concentrator = false)
  {
    double theta_rad = incidence_angle_deg * M_PI / 180.0;

    if (concentrator)
    {
      // Concentrator model
      return 1.0 - b0 * std::pow(std::tan(theta_rad), 2);
    }
    else
    {
      // Flat plate model
      double sec_theta = 1.0 / std::cos(theta_rad);
      return 1.0 - b0 * (sec_theta - 1.0);
    }
  }

  static void PlotIAMComparison(const IAMValidationData &data,
                                const std::string &output_filename)
  {
    plt::figure();

    // Plot both curves
    plt::plot(data.incidence_angle_deg, data.simulated_iam,
              {{"label", "Simulated (Ray Tracing)"}, {"marker", "o"}});
    plt::plot(data.incidence_angle_deg, data.theoretical_iam,
              {{"label", "Theoretical Model"}, {"marker", "s"}, {"linestyle", "--"}});

    plt::title("Incidence Angle Modifier (IAM) Comparison: " + data.collector_type);
    plt::xlabel("Incidence Angle (degrees)");
    plt::ylabel("IAM Factor");
    plt::legend();
    plt::grid(true);
    plt::xlim(0, 90);
    plt::ylim(0, 1.1);

    plt::save(output_filename + "_iam_comparison.png");
    plt::save(output_filename + "_iam_comparison.svg");
    plt::close();

    // Calculate RMSE
    double rmse = 0;
    for (size_t i = 0; i < data.simulated_iam.size(); ++i)
    {
      double diff = data.simulated_iam[i] - data.theoretical_iam[i];
      rmse += diff * diff;
    }
    rmse = std::sqrt(rmse / data.simulated_iam.size());

    std::cout << "\n====== IAM Validation Summary ======\n"
              << "Collector Type: " << data.collector_type << "\n"
              << "Test Points: " << data.incidence_angle_deg.size() << "\n"
              << "RMSE (Simulated vs Theoretical): " << std::fixed << std::setprecision(6)
              << rmse << "\n";
  }
};
