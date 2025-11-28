#include "Collector.hpp"
#include "RayTraceVisualizer.hpp"

void printThermalProps(const RayTraceResult &thp)
{
  std::cout << "SunXmin: " << thp.SunXmin << " [unit]" << std::endl;
  std::cout << "SunXmax: " << thp.SunXmax << " [unit]" << std::endl;
  std::cout << "SunYmin: " << thp.SunYmin << " [unit]" << std::endl;
  std::cout << "SunYmax: " << thp.SunYmax << " [unit]" << std::endl;
  std::cout << "SunRayCount: " << thp.SunRayCount << " [unit]" << std::endl;
  std::cout << "Sun Position: " << thp.Sun_x << ", " << thp.Sun_y << ", " << thp.Sun_z << " [x,y,z]" << std::endl;
  std::cout << "Total Intercepted Ray: " << thp.Length << " [unit]" << std::endl;
  for (const auto &[stage_id, flxmap] : thp.FluxMap)
  {
    std::cout << "================================================== stage id " << stage_id << ": " << flxmap.size() << " ==================================================\n";
    // std::cout << "Xi\tYi\tZi\tXcos\tYcos\tZcos\tElementMap\tRayNumber" << std::endl;
    // for (const auto &mp : flxmap)
    // {
    //   std::cout << mp.X << "," << mp.Y << "," << mp.Z << "," << mp.Xcos << "," << mp.Ycos << ","
    //             << mp.Zcos << "," << mp.ElementMap << "," << mp.RayNumber << "\n";
    // }
  }
  std::cout << std::endl;
}

int main()
{
  auto weather = GeoWeatherData(18.3, 1010.3, 18.9, 11.3);
  auto solar = GeoSolarRadiationData(849.89, 47.0, 127.0, 80.0);
  auto dateTime = GeoDateTimeData("2025-11-21T11:35:01TZ-06");
  auto location = GeoLocationData(51.1507279, -114.1320235, 1150.0, 11);

  std::vector<std::pair<std::string, std::unique_ptr<Collector>>>
      specs;
  specs.emplace_back(std::string("PDC-1"), std::make_unique<ParabolicDish>("Aluminum-Polished", "Copper-Oxidized", 1.50, 1.1, 0.2));
  specs.emplace_back(std::string("PDC-2"), std::make_unique<ParabolicDish>("Aluminum-Polished", "Aluminum-Anodized", 2.50, 2.120, 0.20));
  specs.emplace_back(std::string("PDC-3"), std::make_unique<ParabolicDish>("Copper-Polished", "Steel-Oxidized", 3.50, 2.50, 0.25));
  // specs.emplace_back(std::string("Flat-1"), std::make_unique<FlatPlate>("Iron-Glass", "Graphite-Solid", 2.00, 1.50));
  // specs.emplace_back(std::string("Flat-2"), std::make_unique<FlatPlate>("Iron-Glass", "Steel-Oxidized", 2.00, 1.50));
  // specs.emplace_back(std::string("Flat-3"), std::make_unique<FlatPlate>("Iron-Glass", "Iron-Glass", 2.00, 1.50));

  std::map<std::string, RayTraceResult> ray_results;
  for (const auto &col : specs)
  {
    auto &c = col.second;
    if (!c->IsInitialized())
    {
      std::cerr << col.first << " is not initialized\n";
      continue;
    }
    {
      std::cout << "=========== " << col.first << " thermal properties ===========\n";
      auto result = c->RunAnalysis(dateTime, location, weather, solar);
      printThermalProps(result);
      ray_results[col.first] = result;
    }
  }
  RayPathVisualizer::Plot3DRayPaths(ray_results, "single_loc");
  std::cout << std::endl;
  return 0;
}