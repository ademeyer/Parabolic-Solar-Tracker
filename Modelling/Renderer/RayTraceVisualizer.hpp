#pragma once

#include "matplotlibcpp.h"
#include <vector>
#include <map>
#include <tuple>

namespace plt = matplotlibcpp;

class RayPathVisualizer
{
public:
  static void Plot3DRayPaths(const RayTraceResult &results, const std::string &filename)
  {

    // Create separate plots for each stage
    PlotStage1RayPaths(results, filename);
    PlotStage2RayPaths(results, filename);
    PlotCompleteRayPaths(results, filename);
  }

private:
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

  static void PlotStage1RayPaths(const RayTraceResult &results, const std::string &filename)
  {
    // Extract Stage 1 hits (Parabolic Dish)
    std::vector<double> x1, y1, z1;
    // std::vector<std::vector<double>> dir_x1, dir_y1, dir_z1;
    for (const auto &ray : results.FluxMap)
    {
      if (ray.StageMap == 1)
      { // Stage 1 - Dish
        x1.push_back(ray.X);
        y1.push_back(ray.Y);
        z1.push_back(ray.Z);
      }
    }

    // createRegularGrid(x1, y1, z1, 100, dir_x1, dir_y1, dir_z1);

    std::string fName = filename + "_stage_1_plot.png";

    plt::figure();
    // plt::plot_surface(dir_x1, dir_y1, dir_z1, {{"cmap", "viridis"}});
    plt::scatter(x1, y1, z1, 1.0, {{"c", "teal"}, {"cmap", "viridis"}});
    plt::title("Dish: Flux Distribution");
    plt::xlabel("x-axis (m)");
    plt::ylabel("y-axis (m)");
    plt::set_zlabel("z-axis (m)");
    plt::grid(true);
    plt::save(fName);
    plt::close();
  }

  static double getMagititudeXY(const double &x, const double &y, const double &z = 0.0)
  {
    return static_cast<double>(pow((pow(x, 2.0) + pow(y, 2.0) + pow(z, 2.0)), 0.5));
  }

  static int getColorScaleIndex(const int &color_scale, const double &curVal, const double &min, const double &max)
  {
    return static_cast<int>(roundf((color_scale - 1) * ((curVal - min) / (max - min))));
  }

  static void PlotStage2RayPaths(const RayTraceResult &results, const std::string &filename)
  {
    // Extract Stage 2 hits (Receiver)
    std::vector<double> x2, y2;
    double max_scale = -MAXFLOAT;
    double min_scale = MAXFLOAT;
    std::unordered_map<int, std::pair<std::vector<double>, std::vector<double>>> data_point;

    const std::vector<std::string> colors = {
        "darkblue", // Very Cold
        "blue",
        "deepskyblue", // Cold
        "cyan",
        "aquamarine", // Cool
        "springgreen",
        "lime", // Neutral
        "yellow",
        "gold", // Warm
        "orange",
        "darkorange", // Hot
        "red",
        "darkred", // Very Hot
        "darkmagenta"};

    for (const auto &ray : results.FluxMap)
    {
      if (ray.StageMap == 2)
      { // Stage 2 - Receiver
        max_scale = std::max(max_scale, getMagititudeXY(ray.X, ray.Y, ray.Z));
        min_scale = std::min(min_scale, getMagititudeXY(ray.X, ray.Y, ray.Z));
      }
    }

    auto color_size = colors.size();

    for (const auto &ray : results.FluxMap)
    {
      if (ray.StageMap == 2)
      { // Stage 2 - Receiver
        auto idx = getColorScaleIndex(color_size, getMagititudeXY(ray.X, ray.Y, ray.Z), max_scale, min_scale);
        auto &x_container = data_point[idx].first;
        auto &y_container = data_point[idx].second;
        x_container.push_back(ray.X);
        y_container.push_back(ray.Y);
      }
    }

    std::string fName = filename + "_stage_2_plot.png";
    plt::figure();

    for (int i = 0; i < color_size; ++i)
      plt::scatter(data_point[i].first, data_point[i].second, 1.0, {{"c", colors[i]}});

    plt::title("Receiver: Flux Distribution");
    plt::xlabel("x-axis");
    plt::ylabel("y-axis");
    plt::grid(true);
    plt::save(fName);
    plt::close();
  }

  static void PlotCompleteRayPaths(const RayTraceResult &results, const std::string &filename)
  {
    int max_lint = 350;
    std::vector<double> ray_start_x, ray_start_y, ray_start_z;
    std::vector<double> ray_end_x, ray_end_y, ray_end_z;
    for (const auto &ray : results.FluxMap)
    {
      if (ray.StageMap == 1)
      {
        ray_end_x.push_back(ray.Xcos);
        ray_end_y.push_back(ray.Ycos);
        ray_end_z.push_back(ray.Zcos);
      }
      else
      {
        // origin is dish = stage2
        ray_start_x.push_back(ray.Xcos);
        ray_start_y.push_back(ray.Ycos);
        ray_start_z.push_back(ray.Zcos);
      }
    }

    std::vector<double> ray_x, ray_y, ray_z;

    for (size_t i = 0; i < ray_start_x.size(); ++i)
    {
      if (i > max_lint)
        break;
      // Add start point
      ray_x.push_back(ray_start_x[i]);
      ray_y.push_back(ray_start_y[i]);
      ray_z.push_back(-ray_start_z[i]);

      // Add end point
      ray_x.push_back(ray_end_x[i]);
      ray_y.push_back(ray_end_y[i]);
      ray_z.push_back(-ray_end_z[i]);

      // Add NaN to separate line segments
      if (i < ray_start_x.size() - 1)
      {
        ray_x.push_back(NAN);
        ray_y.push_back(NAN);
        ray_z.push_back(NAN);
      }
    }

    std::string fName = filename + "_stage1_stage2_raytrace.png";
    plt::figure();
    plt::plot3(ray_x, ray_y, ray_z, {{"c", "gold"}, {"linewidth", "0.125"}});
    plt::title("RayTrace: Dish → Receiver");
    plt::xlabel("x-axis (m)");
    plt::ylabel("y-axis (m)");
    plt::set_zlabel("z-axis (m)");
    plt::grid(true);
    plt::save(fName);
    plt::close();
  }
};

class FluxVisualizer
{
public:
  static void PlotFluxHeatmaps(const RayTraceResult &results, const std::string &filename)
  {
    // Stage 1: Incident flux on dish
    PlotStageFluxHeatmap(results, 1, filename + ("Parabolic Dish"), "viridis");

    // Stage 2: Concentrated flux on receiver
    PlotStageFluxHeatmap(results, 2, filename + ("Receiver"), "hot");
  }

private:
  static void PlotStageFluxHeatmap(const RayTraceResult &results, int stage,
                                   const std::string &title, const std::string &colormap)
  {
    std::vector<double> x, y;

    for (const auto &ray : results.FluxMap)
    {
      if (ray.StageMap == stage)
      {
        x.push_back(ray.X);
        y.push_back(ray.Y);
      }
    }

    plt::figure();
    plt::bar(x, y);
    plt::title("stage_" + std::to_string(stage) + std::string(" - Flux Distribution"));
    plt::xlabel("X Position (m)");
    plt::ylabel("Y Position (m)");
    // plt::colorbar();
    plt::grid(true);

    std::string filename = "stage" + std::to_string(stage) + "_flux_heatmap.png";
    plt::save(filename);
    plt::close();
    // plot2DHist(x, stage, title);
    // plot2DHist(y, stage, title);
  }

  static void plot2DHist(const std::vector<double> &x, const int &stage, const std::string &name)
  {
    plt::figure_size(800, 600);

    // 2D histogram (heatmap)
    plt::hist(x, 50, "r");
    plt::title(name + std::string(" - Flux Distribution"));
    plt::xlabel("X Position (m)");
    plt::ylabel("Y Position (m)");
    // plt::colorbar();
    plt::grid(true);

    std::string filename = name + "stage" + std::to_string(stage) + "_flux_heatmap.png";
    plt::save(filename);
    plt::close();
  }

  static void PlotFluxComparison(const RayTraceResult &results)
  {
    // plt::figure_size(1500, 600);

    // // Subplot 1: Stage 1
    // plt::subplot(1, 2, 1);
    // std::vector<double> x1, y1;
    // for (const auto &ray : results.FluxMap)
    // {
    //   if (ray.StageMap == 1)
    //   {
    //     x1.push_back(ray.X);
    //     y1.push_back(ray.Y);
    //   }
    // }
    // plt::hist2d(x1, y1, 50, 50, {{"cmap", "viridis"}});
    // plt::title("Stage 1 - Parabolic Dish");
    // plt::xlabel("X (m)");
    // plt::ylabel("Y (m)");
    // plt::colorbar();

    // // Subplot 2: Stage 2
    // plt::subplot(1, 2, 2);
    // std::vector<double> x2, y2;
    // for (const auto &ray : results.FluxMap)
    // {
    //   if (ray.StageMap == 2)
    //   {
    //     x2.push_back(ray.X);
    //     y2.push_back(ray.Y);
    //   }
    // }
    // plt::hist2d(x2, y2, 50, 50, {{"cmap", "hot"}});
    // plt::title("Stage 2 - Receiver");
    // plt::xlabel("X (m)");
    // plt::ylabel("Y (m)");
    // plt::colorbar();

    // plt::tight_layout();
    // plt::save("flux_comparison.png");
    // plt::close();
  }
};
