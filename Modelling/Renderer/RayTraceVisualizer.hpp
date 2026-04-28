#pragma once

#include "matplotlibcpp.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace plt = matplotlibcpp;
namespace fs = std::filesystem;

fs::path cur = fs::current_path();

static void SaveToFolder(const std::string &dir_name)
{
  fs::path save_dir = cur / "Plots" / dir_name;
  if (!fs::exists(save_dir))
    fs::create_directories(save_dir);

  // switch to save directory
  fs::current_path(save_dir);
}

static const std::vector<std::pair<std::string, int>>
    colors = {             // "color", Temp (K)
        {"darkblue", 300}, // Very Cold
        {"blue", 400},
        {"deepskyblue", 450}, // Cold
        {"cyan", 500},
        {"aquamarine", 550}, // Cool
        {"springgreen", 600},
        {"lime", 650}, // Neutral
        {"yellow", 700},
        {"gold", 750}, // Warm
        {"orange", 800},
        {"darkorange", 850}, // Hot
        {"red", 900},
        {"darkred", 950}, // Very Hot
        {"darkmagenta", 1000}};
class RayPathVisualizer
{
public:
  static void Plot3DRayPaths(const std::map<std::string, RayTraceResult> &results, const std::string &filename)
  {
    if (results.empty())
    {
      std::cerr << "No ray trace results to plot.\n";
      return;
    }
    // Create separate plots for each stage
    PlotDishSurfaceRays(results, filename);
    PlotReceiverFluxDistribution(results, filename);
    PlotCompleteRayPaths(results, filename);
  }

private:
  struct Plots
  {
    std::vector<double> x, y, z;
  };

  static void PlotDishSurfaceRays(const std::map<std::string, RayTraceResult> &results, const std::string &filename)
  {
    // Extract Stage 1 hits (Parabolic Dish)
    std::map<std::string, Plots> dish_plots;
    const int dish_stage_id = 1;

    for (const auto &[dish, ray_result] : results)
    {
      for (const auto &[stage_id, stage_flux] : ray_result.FluxMap)
      {
        if (stage_id == dish_stage_id)
        { // Stage 1 - Dish
          auto &dp = dish_plots[dish];
          for (const auto &ray : stage_flux)
          {
            const auto &[X, Y, Z] = ray.XYZ;
            dp.x.push_back(X);
            dp.y.push_back(Y);
            dp.z.push_back(Z);
          }
        }
      }
    }

    // std::vector<std::vector<double>> d_x, d_y, d_z;
    // createRegularGrid(plots.x, plots.y, plots.z, 100, d_x, d_y, d_z);
    std::string fName = filename + "_dish_surfaceflux.png";
    long plot_num = 1;
    const long plot_size = dish_plots.size();
    const long w = plot_size * 430;
    const long h = 480;

    plt::figure_size(w, h);
    plt::suptitle("Dish Surface Reflection");

    for (const auto &[dish, plots] : dish_plots)
    {
      plt::subplot(1, plot_size, plot_num);
      plt::scatter(plots.x, plots.y, 1.0, {{"c", "gold"}, {"cmap", "inferno"}, {"label", (dish + std::string(": Aperture"))}});
      plt::legend({{"loc", "upper left"}});
      plt::xlabel("x-axis (m)");
      plt::ylabel("y-axis (m)");
      ++plot_num;
    }
    plt::tight_layout();

    SaveToFolder("surface-reflection-plots");

    plt::save(fName);
    plt::close();
  }

  static void PlotReceiverFluxDistribution(const std::map<std::string, RayTraceResult> &results, const std::string &filename)
  {
    /*
    Energy per pixel Is = DNI x Aperture Area / Ns
    flux_pixel = Sum of Ray Energy (DNI x Aperture Area) / A_pixel
    A_pixel = dx x dy = (D_receiver / N)^2
    */
    // Extract Stage 2 hits (Receiver)
    double max_scale = -MAXFLOAT;
    double min_scale = MAXFLOAT;

    const int receiver_stage_id = 2;
    auto color_size = colors.size();
    using datapoint_t = std::unordered_map<int, std::pair<std::vector<double>, std::vector<double>>>; // <color_range, <x,y>>

    std::map<std::string, datapoint_t> data_point;

    for (const auto &[dish, ray_result] : results)
    {
      // Get the min and max point in the dish flux map
      for (const auto &[stage_id, stage_flux] : ray_result.FluxMap)
      {
        if (stage_id == receiver_stage_id)
        { // Stage 2 - Receiver
          for (const auto &ray : stage_flux)
          {
            const auto &[X, Y, Z] = ray.XYZ;
            auto mag = getMagititude(X, Y, Z);
            max_scale = std::max(max_scale, mag);
            min_scale = std::min(min_scale, mag);
          }
        }
      }

      for (const auto &[stage_id, stage_flux] : ray_result.FluxMap)
      {
        if (stage_id == receiver_stage_id)
        { // Stage 2 - Receiver
          for (const auto &ray : stage_flux)
          {
            const auto &[X, Y, Z] = ray.XYZ;
            auto mag = getMagititude(X, Y, Z);
            auto idx = getColorScaleIndex(color_size, mag, max_scale, min_scale);

            auto &dp = data_point[dish];

            auto &x_container = dp[idx].first;
            auto &y_container = dp[idx].second;
            x_container.push_back(X);
            y_container.push_back(Z);
          }
        }
      }
    }

    const int plot_size = data_point.size();
    if (plot_size == 0)
      return;

    int plot_num = 1;
    const long w = plot_size * 430, h = 480;
    std::string fName = filename + "_receiver_surfaceflux.png";
    plt::figure_size(w, h);
    plt::suptitle("Receiver Flux Distribution");

    for (const auto &[dish, dp] : data_point)
    {
      plt::subplot(1, plot_size, plot_num);
      for (const auto &[col, p] : dp)
        plt::scatter(p.first, p.second, 1.0, {{"c", colors[col].first}});

      plt::title(dish, {{"loc", "left"}});
      plt::xlabel("x-axis(m)");
      plt::ylabel("y-axis(m)");
      plt::grid(true);
      ++plot_num;
    }

    SaveToFolder("receiver-flux-distribution");

    plt::save(fName);
    plt::close();
  }

  static void PlotCompleteRayPaths(const std::map<std::string, RayTraceResult> &results, const std::string &filename)
  {
    const int max_line = 750;
    const int dish_stage_id = 1;
    const int receiver_stage_id = 2;

    std::map<std::string, Plots> start, end, start_cos, end_cos;

    auto fillStageMap = [](const std::vector<Ray> &flxmap,
                           Plots &ray,
                           Plots &ray_cos)
    {
      for (const auto &rayMap : flxmap)
      {
        const auto &[X, Y, Z] = rayMap.XYZ;
        const auto &[Xcos, Ycos, Zcos] = rayMap.XYZcos;

        ray_cos.x.push_back(Xcos);
        ray_cos.y.push_back(Ycos);
        ray_cos.z.push_back(Zcos);

        ray.x.push_back(X);
        ray.y.push_back(Y);
        ray.z.push_back(Z);
      }
    };

    for (const auto &[dish, ray_result] : results)
    {
      for (const auto &[stage_id, stage_flxmap] : ray_result.FluxMap)
      {
        if (stage_id == dish_stage_id)
        {
          auto &e = end[dish];
          auto &s = start[dish];
          fillStageMap(stage_flxmap, e, s);
        }
        else if (stage_id == receiver_stage_id)
        {
          auto &e = end_cos[dish];
          auto &s = start_cos[dish];
          fillStageMap(stage_flxmap, e, s);
        }
      }
    }

    if (start.size() != end.size() || start_cos.size() != end_cos.size())
    {
      std::cerr << "Plot count are not equal\n";
      return;
    }

    if (start.size() == 0 || end.size() == 0 || start_cos.size() == 0 || end_cos.size() == 0)
      return;

    std::map<std::string, Plots> ray_, ray_cos;

    auto start_itr = start_cos.begin();
    auto end_itr = end_cos.begin();
    while (start_itr != start_cos.end() && end_itr != end_cos.end())
    {
      const auto &[s_dish, s_plot] = *start_itr;
      const auto &[e_dish, e_plot] = *end_itr;
      // Sanity Check: We have the same PDC name
      if (s_dish != e_dish)
      {
        std::cerr << s_dish << " is not " << e_dish << std::endl;
        return;
      }
      // Sanity Check: start plot vectors are the same size
      if (s_plot.x.size() != s_plot.y.size() || s_plot.x.size() != s_plot.z.size())
      {
        std::cerr << "Ray count in start_cos plots equal\n";
        return;
      }
      // Sanity Check: end plot vectors are the same size
      if (e_plot.x.size() != e_plot.y.size() || e_plot.x.size() != e_plot.z.size())
      {
        std::cerr << "Ray count in end_cos plots equal\n";
        return;
      }

      const auto plot_count = e_plot.x.size();
      const auto &dish = s_dish;
      auto &ray = ray_cos[dish];

      for (size_t i = 0; i < plot_count; ++i)
      {
        if (i >= max_line && max_line > 0)
          break;

        ray.x.push_back(s_plot.x[i]);
        ray.y.push_back(s_plot.y[i]);
        ray.z.push_back(s_plot.z[i]);

        // Add end point
        ray.x.push_back(e_plot.x[i]);
        ray.y.push_back(e_plot.y[i]);
        ray.z.push_back(e_plot.z[i]);

        // Add NaN to separate line segments
        if (i < plot_count - 1)
        {
          ray.x.push_back(NAN);
          ray.y.push_back(NAN);
          ray.z.push_back(NAN);
        }
      }

      ++start_itr;
      ++end_itr;
    }

    end_itr = end.begin();
    while (end_itr != end.end())
    {
      const auto &[e_dish, e_plot] = *end_itr;
      // Sanity Check: end plot vectors are the same size
      if (e_plot.x.size() != e_plot.y.size() || e_plot.x.size() != e_plot.z.size())
      {
        std::cerr << "Ray count in end plots equal\n";
        return;
      }
      // Sanity Check: PDC name exists int RayTraceResult
      if (results.find(e_dish) == results.end())
      {
        std::cerr << e_dish << " is not in RayTraceResult\n";
        return;
      }

      const int plot_count = e_plot.x.size();
      const auto &dish = e_dish;
      auto &ray = ray_[dish];
      const auto &sun_cord = results.at(e_dish);
      const auto &[sun_x, sun_y, sun_z] = sun_cord.SunXYZ;

      for (int i = 0; i < plot_count; ++i)
      {
        if (i >= max_line && max_line > 0)
          break;

        ray.x.push_back(sun_x);
        ray.y.push_back(sun_y);
        ray.z.push_back(sun_z);

        // Add end point
        ray.x.push_back(e_plot.x[i]);
        ray.y.push_back(e_plot.y[i]);
        ray.z.push_back(e_plot.z[i]);

        // Add NaN to separate line segments
        if (i < plot_count - 1)
        {
          ray.x.push_back(NAN);
          ray.y.push_back(NAN);
          ray.z.push_back(NAN);
        }
      }
      ++end_itr;
    }
    plt::suptitle("RayTrace");

    // Plot ray from dish to receiver
    {
      const int plot_size = ray_cos.size();
      if (plot_size == 0)
        return;

      const long w = plot_size * 320, h = 480;
      int plot_num = 1;
      plt::figure_size(w, h);

      std::string fName = filename + "dish-receiver_raytrace.png";

      for (const auto &[dish, plots] : ray_cos)
      {
        plt::subplot(1, plot_size, plot_num);
        plt::plot(plots.y, plots.z, {{"c", "red"}, {"linewidth", "0.125"}, {"label", (dish + std::string(": D → R"))}});
        plt::legend({{"loc", "upper left"}});
        plt::xlabel("y-axis (m)");
        plt::ylabel("z-axis (m)");
        ++plot_num;
      }

      SaveToFolder("dish-receiver-ray-path-plots");

      plt::save(fName);
      plt::close();
    }

    // Plot ray from sun to dish
    {
      const int plot_size = ray_cos.size();
      if (plot_size == 0)
        return;

      int plot_num = 1;
      const long w = plot_size * 320, h = 480;

      plt::figure_size(w, h);

      std::string fName = filename + "sun-dish_raytrace.png";
      for (const auto &[dish, plots] : ray_)
      {
        plt::subplot(1, plot_size, plot_num);
        plt::plot(plots.y, plots.z, {{"c", "gold"}, {"linewidth", "0.125"}, {"label", (dish + std::string(": S → D"))}});
        plt::legend({{"loc", "upper left"}});
        plt::xlabel("y-axis (m)");
        plt::ylabel("z-axis (m)");
        ++plot_num;
      }

      SaveToFolder("sun-dish-ray-path-plots");

      plt::save(fName);
      plt::close();
    }
  }

  static double getMagititude(const double &x, const double &y, const double &z = 0.0)
  {
    return static_cast<double>(pow((pow(x, 2.0) + pow(y, 2.0) + pow(z, 2.0)), 0.5));
  }

  static int getColorScaleIndex(const int &color_scale, const double &curVal, const double &min, const double &max)
  {
    return static_cast<int>(roundf((color_scale - 1) * ((curVal - min) / (max - min))));
  }

  static void createRegularGrid(const std::vector<double> &x,
                                const std::vector<double> &y,
                                const std::vector<double> &z,
                                int grid_size,
                                std::vector<std::vector<double>> &x_grid,
                                std::vector<std::vector<double>> &y_grid,
                                std::vector<std::vector<double>> &z_grid)
  {

    // Find data bounds
    double x_min = *std::min_element(x.begin(), x.end());
    double x_max = *std::max_element(x.begin(), x.end());
    double y_min = *std::min_element(y.begin(), y.end());
    double y_max = *std::max_element(y.begin(), y.end());

    // Create regular grid
    x_grid.resize(grid_size, std::vector<double>(grid_size));
    y_grid.resize(grid_size, std::vector<double>(grid_size));
    z_grid.resize(grid_size, std::vector<double>(grid_size, 0.0));

    // Fill coordinate grids
    for (int i = 0; i < grid_size; ++i)
    {
      for (int j = 0; j < grid_size; ++j)
      {
        x_grid[i][j] = x_min + (x_max - x_min) * i / (grid_size - 1);
        y_grid[i][j] = y_min + (y_max - y_min) * j / (grid_size - 1);
      }
    }

    // Simple nearest neighbor for z values (you might want interpolation)
    for (int i = 0; i < grid_size; ++i)
    {
      for (int j = 0; j < grid_size; ++j)
      {
        double min_dist = std::numeric_limits<double>::max();
        for (size_t k = 0; k < x.size(); ++k)
        {
          double dist = std::hypot(x_grid[i][j] - x[k], y_grid[i][j] - y[k]);
          if (dist < min_dist)
          {
            min_dist = dist;
            z_grid[i][j] = z[k];
          }
        }
      }
    }
  }
};

class ValueVisualizer
{
public:
  static void PlotValueOnBarGraph(const std::map<std::string, std::vector<double>> &plot_values,
                                  const std::vector<std::string> &time_interval,
                                  const std::string &filename,
                                  const std::string &folder_name,
                                  const std::string &y_label,
                                  const std::string &x_label,
                                  const std::string &title)
  {
    const size_t label_interval = time_interval.size();
    const size_t value_size = plot_values.size();
    double max_bar = 0.0;

    if (!value_size || !label_interval)
    {
      std::cerr << "values or label size can not be zero, value_size: " << value_size << ", label_interval: " << label_interval << std::endl;
      return;
    }

    for (const auto &[dish, dish_values] : plot_values)
    {
      if (dish_values.size() != label_interval)
      {
        std::cerr << "Mismatch between labels size: " << label_interval << " and values size: " << dish_values.size() << " for dish: " << dish << std::endl;
        return;
      }
    }

    const double bar_width = 4.8 / value_size; // Dynamic width based on number of dishes
    std::vector<double> x_positions(label_interval);

    for (size_t i = 0; i < label_interval; ++i)
      x_positions[i] = i;

    const auto color_size = colors.size();
    int color_index = 0;
    int dish_index = 0;

    const long w = (plot_values.size() * 320) + 320, h = 480;

    // plt::figure();
    plt::figure_size(w, h);

    for (const auto &[dish, dish_values] : plot_values)
    {
      max_bar = std::max(max_bar, *std::max_element(dish_values.begin(), dish_values.end()));
      std::vector<double> dish_x_positions;

      // Calculate x positions for this dish with proper offset
      for (size_t i = 0; i < label_interval; ++i)
      {
        double offset = (dish_index - (value_size - 1) / 2.0) * bar_width;
        dish_x_positions.push_back(x_positions[i] + offset);
      }

      plt::bar(dish_x_positions, dish_values, colors[color_index % color_size].first, "-", 0.5, {{"label", dish}});
      ++dish_index;
      ++color_index;
    }

    plt::title(title);
    plt::xlabel(x_label);
    plt::ylabel(y_label);

    // Set x-axis ticks and labels
    plt::xticks(x_positions, time_interval);
    plt::legend();
    plt::ylim(0.0, max_bar + (max_bar / 4.0)); // Add some padding to the y-axis
    plt::grid(true);
    std::string pngName = filename + "_" + title + "_" + ".png";
    SaveToFolder(folder_name);

    plt::save(pngName);
    plt::close();
  }

  static void PlotValueOnGraph(
      const std::map<std::string, std::vector<double>> &plot_values,
      const std::vector<std::string> &time_interval,
      const std::string &filename,
      const std::string &folder_name,
      const std::string &y_label,
      const std::string &x_label,
      const std::string &title)
  {
    if (plot_values.empty() || time_interval.empty())
      return;

    const long w = (plot_values.size() * 320) + 320, h = 480;

    // plt::figure();
    plt::figure_size(w, h);

    std::vector<double> x_positions;
    std::vector<std::string> labels;

    for (size_t i = 0; i < time_interval.size(); ++i)
    {
      x_positions.push_back(i);
      labels.push_back(time_interval[i] + ":00");
    }

    for (const auto &[name, values] : plot_values)
    {
      if (values.size() == time_interval.size())
      {
        plt::plot(x_positions, values, {{"label", name}, {"marker", "o"}});
      }
    }

    plt::title(title);
    plt::xlabel(x_label);
    plt::ylabel(y_label);
    plt::xticks(x_positions, labels);
    plt::legend();
    plt::grid(true);
    std::string pngName = filename + "_" + title + "_" + ".png";

    SaveToFolder(folder_name);

    plt::save(pngName);
    plt::close();
  }
};

// /**
//  * @brief Thermal metrics visualization
//  */
// class ThermalVisualizer
// {
// public:
//   /**
//    * @brief Plot thermal efficiency over time
//    */
//   static void PlotThermalEfficiency(
//       const std::map<std::string, std::vector<double>> &thermal_efficiency,
//       const std::vector<std::string> &time_interval,
//       const std::string &filename)
//   {
//     if (thermal_efficiency.empty() || time_interval.empty())
//       return;

//     plt::figure();
//     std::vector<double> x_positions;
//     std::vector<std::string> labels;

//     for (size_t i = 0; i < time_interval.size(); ++i)
//     {
//       x_positions.push_back(i);
//       labels.push_back(time_interval[i] + ":00");
//     }

//     for (const auto &[name, values] : thermal_efficiency)
//     {
//       if (values.size() == time_interval.size())
//       {
//         plt::plot(x_positions, values, {{"label", name}, {"marker", "o"}});
//       }
//     }

//     plt::title("Thermal Efficiency Over Time");
//     plt::xlabel("Time of Day");
//     plt::ylabel("Thermal Efficiency (%)");
//     plt::xticks(x_positions, labels);
//     plt::legend();
//     plt::grid(true);
//     std::string pngName = filename + "_thermal_efficiency.png";

//     SaveToFolder("thermal-efficiency-plots");

//     plt::save(pngName);
//   }

//   /**
//    * @brief Plot operating temperature over time
//    */
//   static void PlotOperatingTemperature(
//       const std::map<std::string, std::vector<double>> &operating_temperature,
//       const std::vector<std::string> &time_interval,
//       const std::string &filename)
//   {
//     if (operating_temperature.empty() || time_interval.empty())
//       return;

//     plt::figure();
//     std::vector<double> x_positions;
//     std::vector<std::string> labels;

//     for (size_t i = 0; i < time_interval.size(); ++i)
//     {
//       x_positions.push_back(i);
//       labels.push_back(time_interval[i] + ":00");
//     }

//     for (const auto &[name, values] : operating_temperature)
//     {
//       if (values.size() == time_interval.size())
//       {
//         plt::plot(x_positions, values, {{"label", name}, {"marker", "s"}});
//       }
//     }

//     plt::title("Operating Temperature Over Time");
//     plt::xlabel("Time of Day");
//     plt::ylabel("Temperature (°C)");
//     plt::xticks(x_positions, labels);
//     plt::legend();
//     plt::grid(true);
//     std::string pngName = filename + "_operating_temperature.png";

//     SaveToFolder("ThermalTemperaturePlots");

//     plt::save(pngName);
//   }

//   /**
//    * @brief Plot useful energy output over time
//    */
//   static void PlotUsefulEnergyOutput(
//       const std::map<std::string, std::vector<double>> &useful_energy,
//       const std::vector<std::string> &time_interval,
//       const std::string &filename)
//   {
//     if (useful_energy.empty() || time_interval.empty())
//       return;

//     plt::figure();
//     std::vector<double> x_positions;
//     std::vector<std::string> labels;

//     for (size_t i = 0; i < time_interval.size(); ++i)
//     {
//       x_positions.push_back(i);
//       labels.push_back(time_interval[i] + ":00");
//     }

//     for (const auto &[name, values] : useful_energy)
//     {
//       if (values.size() == time_interval.size())
//       {
//         plt::plot(x_positions, values, {{"label", name}, {"marker", "^"}});
//       }
//     }

//     plt::title("Useful Energy Output Over Time");
//     plt::xlabel("Time of Day");
//     plt::ylabel("Energy Output (kW)");
//     plt::xticks(x_positions, labels);
//     plt::legend();
//     plt::grid(true);
//     std::string pngName = filename + "_useful_energy.png";

//     SaveToFolder("useful-energy-plots");

//     plt::save(pngName);
//   }

//   /**
//    * @brief Plot thermal loss breakdown (stacked bar chart)
//    */
//   static void PlotThermalLossBreakdown(
//       const std::map<std::string, std::vector<std::array<double, 3>>> &loss_breakdown,
//       const std::vector<std::string> &time_interval,
//       const std::string &filename)
//   {
//     if (loss_breakdown.empty() || time_interval.empty())
//       return;

//     plt::figure();
//     std::vector<double> x_positions;
//     std::vector<std::string> labels;

//     for (size_t i = 0; i < time_interval.size(); ++i)
//     {
//       x_positions.push_back(i);
//       labels.push_back(time_interval[i] + ":00");
//     }

//     // Plot stacked bars for each collector
//     size_t collector_idx = 0;
//     const int color_size = colors.size();
//     for (const auto &[name, losses_data] : loss_breakdown)
//     {
//       if (losses_data.size() != time_interval.size())
//         continue;

//       std::vector<double> radiation, convection, conduction;
//       for (const auto &loss : losses_data)
//       {
//         radiation.push_back(loss[0]);
//         convection.push_back(loss[1]);
//         conduction.push_back(loss[2]);
//       }

//       // Use bar plot with stacking
//       plt::bar(x_positions, radiation, colors[collector_idx % color_size].first, "-", 0.6, {{"label", name + ": Radiation"}});
//       collector_idx++;
//     }

//     plt::title("Thermal Loss Breakdown");
//     plt::xlabel("Time of Day");
//     plt::ylabel("Loss (kW)");
//     plt::xticks(x_positions, labels);
//     plt::legend();
//     plt::grid(true);
//     std::string pngName = filename + "_loss_breakdown.png";

//     SaveToFolder("thermal-loss-breakdown-plots");

//     plt::save(pngName);
//   }
// };