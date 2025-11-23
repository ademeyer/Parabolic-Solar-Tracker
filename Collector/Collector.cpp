#include "Collector.hpp"

ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                             const double &reactor_length)
{
  if (dish_diameter <= 0.0 || dish_depth <= 0.0 || reactor_width <= 0.0)
  {
    throw std::invalid_argument("Need dimensions to be positive number");
  }

  m_DishDimension = std::make_unique<Parabola>(dish_diameter, dish_depth);

  reactor_length == 0 ? m_ReactorDimension = std::make_unique<Parabola>(reactor_width)
                      : m_ReactorDimension = std::make_unique<FlatSurface>(reactor_width, reactor_length);

  reactor_length == 0 ? m_ReceiverDiameter = reactor_width : (m_ReceiverDiameter = pow(((pow(reactor_length, 2) + pow(reactor_width, 2))), 0.5));

  // Initialize both dish and reactor material
  MaterialProperties matProp;
  Material dishMat, reactorMat;
  if (!matProp.FetchMaterial(dish_material_name, dishMat))
  {
    std::cerr << "Material name not found in material.conf: " << dish_material_name << std::endl;
    return;
  }
  m_DishMaterial = std::make_unique<Material>(dishMat);

  if (!matProp.FetchMaterial(reactor_material_name, reactorMat))
  {
    std::cerr << "Material name not found in material.conf: " << reactor_material_name << std::endl;
    return;
  }
  m_ReactorMaterial = std::make_unique<Material>(reactorMat);

  // Initialize Collector losses
  if (m_ReactorDimension)
  {
    m_ConvectiveLoss = std::make_unique<ConvectiveLoss>(m_ReactorDimension->GetArea());
  }

  // Initialize SolTrace Model
  auto dim = dynamic_cast<Parabola *>(m_DishDimension.get());
  if (!dim)
    throw std::runtime_error("Downcasting Failed\n");
  std::cout << "focal length: " << dim->GetFocalLength() << std::endl;
  std::vector<CollectorSpecs> specs = {
      CollectorSpecs("PabolarDish", dish_diameter, dish_diameter, m_DishMaterial->reflectivity, m_DishMaterial->absorptivity, 0.0, dim->GetFocalLength()),
      CollectorSpecs("ReactorPlate", reactor_width, reactor_length, m_ReactorMaterial->reflectivity, m_ReactorMaterial->absorptivity, 0.0, dim->GetFocalLength())};
  m_STraceModel = std::make_unique<SolTraceModel>(specs);
}

RayTraceResult ParabolicDish::RunAnalysis(const GeoDateTimeData &dataTime,
                                          const GeoLocationData &gLocation,
                                          const GeoWeatherData &weather,
                                          const GeoSolarRadiationData &solar_rad) const
{
  auto intercepted_q = intercepted_energy(solar_rad);
  SPA_Input spa_in(dataTime, gLocation, weather);

  auto spa_data = getSunPosition(&spa_in);
  if (!spa_data.IsValid())
  {
    std::cerr << "Invalid sun position data: Error: " << spa_data.errCode << std::endl;
    return {};
  }

  /* Estimate ray number based on the amount of direct radiation received */
  int ray_num = solar_rad.DNI * RAY_NUM_MAX / SOLAR_CONSTANT;

  return m_STraceModel->RunAnalysis(spa_data.azimuth, spa_data.elevation, ray_num);
}

ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width)
    : ParabolicDish(dish_material_name, reactor_material_name, dish_diameter, dish_depth, reactor_width, 0.0)
{
}

bool ParabolicDish::IsInitialized() const
{
  return (m_STraceModel && m_DishDimension && m_ReactorDimension && m_DishMaterial && m_ReactorMaterial && m_ConvectiveLoss);
}

double ParabolicDish::geometric_ratio() const
{
  /* ratio of area of dish to reactor */
  return m_DishDimension->GetArea() / m_ReactorDimension->GetArea();
}

double ParabolicDish::intercepted_energy(const GeoSolarRadiationData &solar_rad) const
{
  /**
   *  total energy received by the reactor:
   * (direct normal radiation * aperture area * G.R)
   */
  return solar_rad.DNI * m_DishDimension->GetArea() * geometric_ratio();
}

/********************** FlatPlate Collector ***************************/
FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name,
                     const double &surface_width, const double &surface_length, const double &absorber_width,
                     const double &absorber_length)
{
  m_SurfaceDimension = std::make_unique<FlatSurface>(surface_width, surface_length);
  m_AbsorberDimension = std::make_unique<FlatSurface>(absorber_width, absorber_length);

  // Initialize surface and absorber material
  MaterialProperties matProp;
  Material surfaceMat, absorberMat;
  if (!matProp.FetchMaterial(surface_material_name, surfaceMat))
  {
    std::cerr << "Material name not found in material.conf: " << surface_material_name << std::endl;
    return;
  }
  m_SurfaceMaterial = std::make_unique<Material>(surfaceMat);

  if (!matProp.FetchMaterial(absorber_material_name, absorberMat))
  {
    std::cerr << "Material name not found in material.conf: " << absorber_material_name << std::endl;
    return;
  }
  m_AbsorberMaterial = std::make_unique<Material>(absorberMat);
  // m_AbsorberMaterial->DumpInfo();

  // Initialize Collector losses
  if (m_AbsorberDimension)
  {
    m_ConvectiveLoss = std::make_unique<ConvectiveLoss>(m_AbsorberDimension->GetArea());

    if (m_AbsorberMaterial)
      m_ConductionLoss = std::make_unique<ConductionLoss>(m_AbsorberMaterial->thermal_conductivity,
                                                          m_AbsorberMaterial->thickness,
                                                          m_AbsorberDimension->GetArea());
  }
}

FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name,
                     const double &surface_width, const double &surface_length)
    : FlatPlate(surface_material_name, absorber_material_name, surface_width, surface_length,
                surface_width, surface_length) {}

RayTraceResult FlatPlate::RunAnalysis(const GeoDateTimeData &dataTime,
                                      const GeoLocationData &gLocation,
                                      const GeoWeatherData &weather,
                                      const GeoSolarRadiationData &solar_rad) const
{
  auto intercepted_q = intercepted_energy(solar_rad);
  // auto Noptical = optical_efficiency(cosine_angle);
  // auto max_temp = temperature(intercepted_q, m_AbsorberMaterial->emissivity, m_AbsorberDimension->GetArea());
  // auto op_temp = temperature((intercepted_q * Noptical), m_AbsorberMaterial->emissivity, m_AbsorberDimension->GetArea());

  return {};
}

bool FlatPlate::IsInitialized() const
{
  return (m_SurfaceDimension && m_AbsorberDimension && m_SurfaceMaterial &&
          m_AbsorberMaterial && m_ConductionLoss && m_ConvectiveLoss);
}

double FlatPlate::intercepted_energy(const GeoSolarRadiationData &solar_rad) const
{
  /**
   *  total energy received by the surface:
   * (direct normal radiation * aperture area)
   */
  return solar_rad.DNI * m_SurfaceDimension->GetArea();
}