#pragma once

#include "matplotlibcpp.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <tuple>

namespace plt = matplotlibcpp;

class RayPathVisualizer
{
public:
  static void Plot3DRayPaths(const RayTraceResult &results, const std::string &filename)
  {
    // Create separate plots for each stage
    PlotDishSurfaceRays(results, filename);
    PlotReceiverFluxDistribution(results, filename);
    PlotCompleteRayPaths(results, filename);
  }

private:
  static void PlotDishSurfaceRays(const RayTraceResult &results, const std::string &filename)
  {
    // Extract Stage 1 hits (Parabolic Dish)
    std::vector<double> x1, y1, z1;
    const int dish_stage_id = 1;
    // std::vector<std::vector<double>> dir_x1, dir_y1, dir_z1;
    for (const auto &[stage_id, stage_flux] : results.FluxMap)
    {
      if (stage_id == dish_stage_id)
      { // Stage 1 - Dish
        for (const auto &ray : stage_flux)
        {
          x1.push_back(ray.X);
          y1.push_back(ray.Y);
          z1.push_back(ray.Z);
        }
      }
    }

    // createRegularGrid(x1, y1, z1, 100, dir_x1, dir_y1, dir_z1);

    std::string fName = filename + "_dish_surfaceflux.png";

    plt::figure();
    // plt::plot_surface(dir_x1, dir_y1, dir_z1, {{"cmap", "viridis"}});
    plt::scatter(x1, y1, z1, 1.0, {{"c", "lightgoldenrodyellow"}, {"cmap", "inferno"}});
    plt::title("Dish Surface Flux Distribution");
    plt::xlabel("x-axis (m)");
    plt::ylabel("y-axis (m)");
    plt::set_zlabel("z-axis (m)");
    plt::grid(true);
    plt::save(fName);
    plt::close();
  }

  static void PlotReceiverFluxDistribution(const RayTraceResult &results, const std::string &filename)
  {
    /*
    Energy per pixel Is = DNI x Aperture Area / Ns
    flux_pixel = Sum of Ray Energy (DNI x Aperture Area) / A_pixel
    A_pixel = dx x dy = (D_receiver / N)^2
    */
    // Extract Stage 2 hits (Receiver)
    std::vector<double> x2, y2;
    double max_scale = -MAXFLOAT;
    double min_scale = MAXFLOAT;
    std::unordered_map<int, std::pair<std::vector<double>, std::vector<double>>> data_point;

    const std::vector<std::pair<std::string, int>>
        colors = {                   // "color", Temp (K)
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

    const int receiver_stage_id = 2;

    for (const auto &[stage_id, stage_flux] : results.FluxMap)
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

    auto color_size = colors.size();

    for (const auto &[stage_id, stage_flux] : results.FluxMap)
    {
      if (stage_id == receiver_stage_id)
      { // Stage 2 - Receiver
        for (const auto &ray : stage_flux)
        {
          auto mag = getMagititude(ray.X, ray.Y, ray.Z);
          auto idx = getColorScaleIndex(color_size, mag, max_scale, min_scale);
          auto &x_container = data_point[idx].first;
          auto &y_container = data_point[idx].second;
          x_container.push_back(ray.X);
          y_container.push_back(ray.Y);
        }
      }
    }

    std::string fName = filename + "_receiver_surfaceflux.png";
    plt::figure();

    for (size_t i = 0; i < color_size; ++i)
      plt::scatter(data_point[i].first, data_point[i].second, 1.0, {{"c", colors[i].first}, {"label", (std::to_string(colors[i].second) + "K")}});

    plt::legend({{"loc", "upper left"}});
    plt::title("Receiver Flux Distribution");
    plt::xlabel("x-axis(m)");
    plt::ylabel("y-axis(m)");
    plt::grid(true);
    plt::save(fName);
    plt::close();
  }

  static void PlotCompleteRayPaths(const RayTraceResult &results, const std::string &filename)
  {
    const int max_lint = 350;
    std::vector<double> ray_start_x, ray_start_y, ray_start_z;
    std::vector<double> ray_end_x, ray_end_y, ray_end_z;
    std::vector<double> ray_start_x_cos, ray_start_y_cos, ray_start_z_cos;
    std::vector<double> ray_end_x_cos, ray_end_y_cos, ray_end_z_cos;

    const int dish_stage_id = 1;
    const int receiver_stage_id = 2;

    auto fillStageMap = [](const std::vector<RayMap> &flxmap,
                           std::vector<double> &ray_x,
                           std::vector<double> &ray_y,
                           std::vector<double> &ray_z,
                           std::vector<double> &ray_xcos,
                           std::vector<double> &ray_ycos,
                           std::vector<double> &ray_zcos)
    {
      for (const auto &rayMap : flxmap)
      {
        ray_xcos.push_back(rayMap.Xcos);
        ray_ycos.push_back(rayMap.Ycos);
        ray_zcos.push_back(rayMap.Zcos);

        ray_x.push_back(rayMap.X);
        ray_y.push_back(rayMap.Y);
        ray_z.push_back(rayMap.Z);
      }
    };

    for (const auto &[stage_id, stage_flxmap] : results.FluxMap)
    {
      if (stage_id == dish_stage_id)
      {
        fillStageMap(stage_flxmap,
                     ray_end_x,
                     ray_end_y,
                     ray_end_z,
                     ray_end_x_cos,
                     ray_end_y_cos,
                     ray_end_z_cos);
      }
      else if (stage_id == receiver_stage_id)
      {
        fillStageMap(stage_flxmap,
                     ray_start_x,
                     ray_start_y,
                     ray_start_z,
                     ray_start_x_cos,
                     ray_start_y_cos,
                     ray_start_z_cos);
      }
    }

    std::vector<double> ray_x, ray_y, ray_z;
    for (size_t i = 0; i < ray_end_x.size(); ++i)
    {
      if (i >= max_lint)
        break;

      // Add start point
      ray_x.push_back(results.Sun_x);
      ray_y.push_back(results.Sun_y);
      ray_z.push_back(-results.Sun_z);

      // Add end point
      ray_x.push_back(ray_end_x[i]);
      ray_y.push_back(ray_end_y[i]);
      ray_z.push_back(-ray_end_z[i]);

      // Add NaN to separate line segments
      if (i < ray_end_x.size() - 1)
      {
        ray_x.push_back(NAN);
        ray_y.push_back(NAN);
        ray_z.push_back(NAN);
      }
    }

    std::vector<double> ray_x_cos, ray_y_cos, ray_z_cos;
    for (size_t i = 0; i < ray_start_x_cos.size(); ++i)
    {
      if (i >= max_lint)
        break;

      // cosine
      ray_x_cos.push_back(ray_start_x_cos[i]);
      ray_y_cos.push_back(ray_start_y_cos[i]);
      ray_z_cos.push_back(-ray_start_z_cos[i]);

      // Add end point
      ray_x_cos.push_back(ray_end_x_cos[i]);
      ray_y_cos.push_back(ray_end_y_cos[i]);
      ray_z_cos.push_back(-ray_end_z_cos[i]);

      // Add NaN to separate line segments
      if (i < ray_start_x_cos.size() - 1)
      {
        ray_x_cos.push_back(NAN);
        ray_y_cos.push_back(NAN);
        ray_z_cos.push_back(NAN);
      }
    }

    // Plot ray from dish to receiver
    {
      std::string fName = filename + "_raytrace_dish-receiver.png";

      plt::plot3(ray_x_cos, ray_y_cos, ray_z_cos, {{"c", "red"}, {"linewidth", "0.125"}, {"label", "Ray Lines"}});
      plt::legend({{"loc", "upper left"}});

      plt::title("RayTrace: Dish → Receiver");
      plt::xlabel("x-axis (m)");
      plt::ylabel("y-axis (m)");
      plt::set_zlabel("z-axis (m)");
      plt::grid(true);
      plt::save(fName);
      plt::close();
    }
    // // plot ray from sun to dish
    // {
    //   std::string fName = filename + "_raytrace_sun-dish.png";

    //   plt::plot3(ray_x, ray_y, ray_z, {{"c", "gold"}, {"linewidth", "0.125"}, {"label", "Ray Lines"}});
    //   plt::legend({{"loc", "upper left"}});

    //   plt::title("RayTrace: Sun → Dish");
    //   plt::xlabel("x-axis (m)");
    //   plt::ylabel("y-axis (m)");
    //   plt::set_zlabel("z-axis (m)");
    //   plt::grid(true);
    //   plt::save(fName);
    //   plt::close();
    // }
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