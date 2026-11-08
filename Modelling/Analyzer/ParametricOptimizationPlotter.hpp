#pragma once
#include <vector>
#include <string>
#include <cmath>
#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

/**
 * @brief Parametric Optimization Plotter
 * Generates 3D response surfaces and contour plots for optimization analysis
 */
class ParametricOptimizationPlotter
{
public:
  /**
   * @brief Plot 3D response surface: Thermal efficiency vs. (Dish diameter, Focal length)
   */
  static void Plot3DResponseSurface(
      const std::vector<double> &dish_diameters,
      const std::vector<double> &focal_lengths,
      const std::vector<std::vector<double>> &thermal_efficiency_grid,
      const std::string &output_filename)
  {
    if (dish_diameters.empty() || focal_lengths.empty() ||
        thermal_efficiency_grid.empty())
      return;

    plt::figure();

    // Prepare data for surface plot
    // Note: matplotlib-cpp has limited 3D support, so we use plot_surface via Python dict
    py::dict kwargs = py::dict();
    kwargs["cmap"] = "viridis";
    kwargs["alpha"] = 0.8;

    // Create meshgrid
    std::vector<std::vector<double>> X, Y, Z;
    CreateMeshGrid(dish_diameters, focal_lengths, thermal_efficiency_grid, X, Y, Z);

    // Plot using matplotlib
    auto ax = plt::subplot(1, 1, 1, py::dict(py::arg("projection") = "3d"));

    plt::title("Thermal Efficiency Response Surface\n(Dish Diameter vs Focal Length)");
    plt::xlabel("Dish Diameter (m)");
    plt::ylabel("Focal Length (m)");

    // Surface plot
    py::object surf = ax.attr("plot_surface")(X, Y, Z, kwargs);

    plt::tight_layout();
    plt::save(output_filename + "_3d_response_surface.png");
    plt::save(output_filename + "_3d_response_surface.svg");
    plt::close();
  }

  /**
   * @brief Plot 2D contour plot for easier reading
   */
  static void PlotContourMap(
      const std::vector<double> &dish_diameters,
      const std::vector<double> &focal_lengths,
      const std::vector<std::vector<double>> &thermal_efficiency_grid,
      const std::string &output_filename)
  {
    if (dish_diameters.empty() || focal_lengths.empty() ||
        thermal_efficiency_grid.empty())
      return;

    plt::figure();

    // Create meshgrid
    std::vector<std::vector<double>> X, Y, Z;
    CreateMeshGrid(dish_diameters, focal_lengths, thermal_efficiency_grid, X, Y, Z);

    // Contour plot
    plt::contourf(X, Y, Z, {{"levels", 20}, {"cmap", "viridis"}});
    plt::colorbar();

    plt::title("Thermal Efficiency Contour Map\n(Dish Diameter vs Focal Length)");
    plt::xlabel("Dish Diameter (m)");
    plt::ylabel("Focal Length (m)");
    plt::grid(true);

    plt::tight_layout();
    plt::save(output_filename + "_contour_map.png");
    plt::save(output_filename + "_contour_map.svg");
    plt::close();
  }

  /**
   * @brief Plot optimization landscape with marked optimum
   */
  static void PlotOptimizationLandscape(
      const std::vector<double> &x_values,
      const std::vector<double> &y_values,
      const std::string &x_label,
      const std::string &y_label,
      const std::string &output_filename)
  {
    if (x_values.empty() || y_values.empty())
      return;

    plt::figure();

    // Find maximum
    size_t max_idx = 0;
    for (size_t i = 1; i < y_values.size(); ++i)
    {
      if (y_values[i] > y_values[max_idx])
        max_idx = i;
    }

    // Plot landscape
    plt::plot(x_values, y_values, {{"marker", "o"}, {"label", "Performance"}});

    // Mark optimum
    plt::scatter({x_values[max_idx]}, {y_values[max_idx]},
                 100.0, {{"marker", "*"}, {"c", "red"}, {"label", "Optimum"}});

    plt::title("Performance Optimization Landscape");
    plt::xlabel(x_label);
    plt::ylabel(y_label);
    plt::legend();
    plt::grid(true);

    plt::tight_layout();
    plt::save(output_filename + "_optimization_landscape.png");
    plt::save(output_filename + "_optimization_landscape.svg");
    plt::close();
  }

  /**
   * @brief Plot multi-parameter sensitivity surface
   * Shows how output varies with two input parameters
   */
  static void PlotSensitivitySurface(
      const std::vector<double> &param1_values,
      const std::vector<double> &param2_values,
      const std::vector<std::vector<double>> &output_grid,
      const std::string &param1_name,
      const std::string &param2_name,
      const std::string &output_name,
      const std::string &output_filename)
  {
    if (param1_values.empty() || param2_values.empty() ||
        output_grid.empty())
      return;

    plt::figure();

    // Create meshgrid
    std::vector<std::vector<double>> X, Y, Z;
    CreateMeshGrid(param1_values, param2_values, output_grid, X, Y, Z);

    // Contour plot with colorbar
    plt::contourf(X, Y, Z, {{"levels", 15}, {"cmap", "RdYlGn"}});
    plt::colorbar();

    plt::title("Parameter Sensitivity Analysis");
    plt::xlabel(param1_name);
    plt::ylabel(param2_name);

    // Add contour lines
    plt::contour(X, Y, Z, {{"levels", 10}, {"colors", "black"}, {"alpha", 0.3}});

    plt::grid(true);
    plt::tight_layout();
    plt::save(output_filename + "_sensitivity_surface.png");
    plt::save(output_filename + "_sensitivity_surface.svg");
    plt::close();
  }

private:
  /**
   * @brief Create meshgrid for surface plotting
   */
  static void CreateMeshGrid(
      const std::vector<double> &x_vals,
      const std::vector<double> &y_vals,
      const std::vector<std::vector<double>> &z_vals,
      std::vector<std::vector<double>> &X,
      std::vector<std::vector<double>> &Y,
      std::vector<std::vector<double>> &Z)
  {
    X.resize(x_vals.size());
    Y.resize(x_vals.size());
    Z = z_vals; // Copy z values

    for (size_t i = 0; i < x_vals.size(); ++i)
    {
      X[i].resize(y_vals.size());
      Y[i].resize(y_vals.size());

      for (size_t j = 0; j < y_vals.size(); ++j)
      {
        X[i][j] = x_vals[i];
        Y[i][j] = y_vals[j];
      }
    }
  }
};
