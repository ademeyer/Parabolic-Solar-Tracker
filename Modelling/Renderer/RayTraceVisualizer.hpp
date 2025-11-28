#pragma once

#include "matplotlibcpp.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>

namespace plt = matplotlibcpp;

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
            dp.x.push_back(ray.X);
            dp.y.push_back(ray.Y);
            dp.z.push_back(ray.Z);
          }
        }
      }
    }

    // std::vector<std::vector<double>> d_x, d_y, d_z;
    // createRegularGrid(plots.x, plots.y, plots.z, 100, d_x, d_y, d_z);
    std::string fName = filename + "_dish_surfaceflux.png";
    int plot_num = 1;
    const int plot_size = dish_plots.size();

    plt::figure_size(1920, 640);
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
            auto mag = getMagititude(ray.X, ray.Y, ray.Z);
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
            auto mag = getMagititude(ray.X, ray.Y, ray.Z);
            auto idx = getColorScaleIndex(color_size, mag, max_scale, min_scale);

            auto &dp = data_point[dish];

            auto &x_container = dp[idx].first;
            auto &y_container = dp[idx].second;
            x_container.push_back(ray.X);
            y_container.push_back(ray.Y);
          }
        }
      }
    }

    const int plot_size = data_point.size();
    if (plot_size == 0)
      return;

    int plot_num = 1;
    const long w = plot_size * 720, h = plot_size * 240;
    std::string fName = filename + "_receiver_surfaceflux.png";
    plt::figure_size(w, h);
    plt::suptitle("Receiver Flux Distribution");

    for (const auto &[dish, dp] : data_point)
    {
      plt::subplot(1, plot_size, plot_num);
      for (size_t i = 0; i < dp.size(); ++i)
      {
        auto &p = dp.at(i);
        plt::scatter(p.first, p.second, 1.0, {{"c", colors[i].first}});
      }
      plt::title(dish, {{"loc", "left"}});
      plt::xlabel("x-axis(m)");
      plt::ylabel("y-axis(m)");
      plt::grid(true);
      ++plot_num;
    }

    plt::save(fName);
    plt::close();
  }

  static void PlotCompleteRayPaths(const std::map<std::string, RayTraceResult> &results, const std::string &filename)
  {
    const int max_line = 750;
    const int dish_stage_id = 1;
    const int receiver_stage_id = 2;

    std::map<std::string, Plots> start, end, start_cos, end_cos;

    auto fillStageMap = [](const std::vector<RayMap> &flxmap,
                           Plots &ray,
                           Plots &ray_cos)
    {
      for (const auto &rayMap : flxmap)
      {
        ray_cos.x.push_back(rayMap.Xcos);
        ray_cos.y.push_back(rayMap.Ycos);
        ray_cos.z.push_back(rayMap.Zcos);

        ray.x.push_back(rayMap.X);
        ray.y.push_back(rayMap.Y);
        ray.z.push_back(rayMap.Z);
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
      const auto &sun_x = sun_cord.Sun_x;
      const auto &sun_y = sun_cord.Sun_y;
      const auto &sun_z = sun_cord.Sun_z;

      for (size_t i = 0; i < plot_count; ++i)
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

      const long w = plot_size * 620, h = plot_size * 180;
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
      plt::save(fName);
      plt::close();
    }

    // Plot ray from sun to dish
    {
      const int plot_size = ray_cos.size();
      if (plot_size == 0)
        return;

      const long w = plot_size * 620, h = plot_size * 180;
      int plot_num = 1;
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
  static void PlotValueOnBarGraph(const std::map<std::string, std::vector<double>> &values,
                                  const std::vector<std::string> &labels,
                                  const std::string &filename)
  {
    const size_t label_interval = labels.size();
    const size_t value_size = values.size();
    if (!value_size || !label_interval)
    {
      std::cerr << "values or label size can not be zero\n";
      return;
    }

    for (const auto &[dish, dish_values] : values)
    {
      if (dish_values.size() != label_interval)
      {
        std::cerr << "Mismatch between labels size and values size for dish: " << dish << std::endl;
        return;
      }
    }

    const double bar_width = 2.5 / value_size; // Dynamic width based on number of dishes
    std::vector<double> x_positions(label_interval);

    for (size_t i = 0; i < label_interval; ++i)
      x_positions[i] = i;

    const auto color_size = colors.size();
    int color_index = 0;
    int dish_index = 0;

    plt::figure();

    for (const auto &[dish, dish_values] : values)
    {

      std::vector<double> dish_x_positions;

      // Calculate x positions for this dish with proper offset
      for (size_t i = 0; i < label_interval; ++i)
      {
        double offset = (dish_index - (value_size - 1) / 2.0) * bar_width;
        dish_x_positions.push_back(x_positions[i] + offset);
      }

      plt::bar(dish_x_positions, dish_values, colors[color_index % colors.size()].first, "-", 0.25, {{"label", dish}});
      ++dish_index;
      ++color_index;
    }

    plt::title("Optical Efficiency Plot");
    plt::xlabel("Time of Day");
    plt::ylabel("Efficiency (%)");

    // Set x-axis ticks and labels
    plt::xticks(x_positions, labels);
    plt::legend();
    plt::ylim(0, 100);
    plt::grid(true);
    std::string fName = filename + "_efficiency.png";
    plt::save(fName);
  }
};