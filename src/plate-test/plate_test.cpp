#include "Collector.hpp"
#include "RayTraceVisualizer.hpp"

double GetOpticalEfficiencyFromRayResult(const RayTraceResult &ray_result,
                                         const double &absorptivity = 1)
{
  const int dish_id = 1, receiver_id = 2;
  int Nr = 0, Nd = 0;

  for (const auto &[id, flxmap] : ray_result.FluxMap)
  {
    if (id == dish_id)
      Nd = flxmap.size();
    else if (id == receiver_id)
      Nr = flxmap.size();
  }

  if (Nd == 0)
  {
    std::cerr << "Dish Ray Hits can not be zero\n";
    return 0.0;
  }
  return static_cast<double>((Nr / (double)Nd) * 100 * absorptivity);
}

void printThermalProps(const RayTraceResult &thp)
{
  std::cout << "SunMin: " << thp.SunMin.X << ", " << thp.SunMin.Y << " [X, Y]" << std::endl;
  std::cout << "SunMax: " << thp.SunMax.X << ", " << thp.SunMax.Y << " [X, Y]" << std::endl;
  std::cout << "SunRayCount: " << thp.SunRayCount << " [unit]" << std::endl;
  std::cout << "Sun Position: " << thp.SunXYZ.X << ", " << thp.SunXYZ.Y << ", " << thp.SunXYZ.Z << " [X, Y, Z]" << std::endl;
  std::cout << "Total Intercepted Ray: " << thp.Length << " [unit]" << std::endl;
  for (const auto &[stage_id, flxmap] : thp.FluxMap)
  {
    std::cout << "================================================== stage id " << stage_id << ": " << flxmap.size() << " ==================================================\n";
    // std::cout << "Xi\tYi\tZi\tXcos\tYcos\tZcos\tElementMap\tRayNumber" << std::endl;
    // for (const auto &mp : flxmap)
    // {
    //   const auto &[X, Y, Z] = mp.XYZ;
    //   const auto &[Xcos, Ycos, Zcos] = mp.XYZcos;
    //   std::cout << X << "," << Y << "," << Z << "," << Xcos << "," << Ycos << ","
    //             << Zcos << "," << mp.ElementMap << "," << mp.RayNumber << "\n";
    // }
  }
  std::cout << std::endl;
}

/*

*/

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
  specs.emplace_back(std::string("Flat-1"), std::make_unique<FlatPlate>("Iron-Glass", "Graphite-Solid", 2.00, 1.50));
  specs.emplace_back(std::string("Flat-2"), std::make_unique<FlatPlate>("Iron-Glass", "Steel-Oxidized", 2.00, 1.50));
  specs.emplace_back(std::string("Flat-3"), std::make_unique<FlatPlate>("Iron-Glass", "Iron-Glass", 2.00, 1.50));

  std::map<std::string, RayTraceResult> ray_results;
  std::map<std::string, std::vector<double>> eff;
  for (const auto &col : specs)
  {
    const auto &c = col.second;
    if (!c->IsInitialized())
    {
      std::cerr << col.first << " is not initialized\n";
      continue;
    }

    {
      std::cout << "=========== " << col.first << " thermal properties ===========\n";
      const auto result = c->RunAnalysis(dateTime, location, weather, solar);
      printThermalProps(result);
      ray_results[col.first] = result;
      const auto &absorberMat = c->GetAbsorberMaterial();
      auto &e = eff[col.first];
      e.push_back(GetOpticalEfficiencyFromRayResult(result, absorberMat.absorptivity));
    }
  }
  RayPathVisualizer::Plot3DRayPaths(ray_results, "single_loc");
  ValueVisualizer::PlotValueOnBarGraph(eff, {"12.00PM"}, "test");
  std::cout << std::endl;
  return 0;
}