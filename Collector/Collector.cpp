#include "Collector.hpp"

/*************************************************************** Collector() Definition *********************************************************************/

Collector::Collector(const std::string &dish_material_name, const std::string &reactor_material_name)
{
  // Initialize both dish and reactor material
  MaterialProperties matProp;
  Material dishMat, reactorMat;
  if (!matProp.FetchMaterial(dish_material_name, dishMat))
  {
    std::cerr << "Material name not found in material.conf: " << dish_material_name << std::endl;
    return;
  }
  m_ReflectiveMaterial = std::make_unique<Material>(dishMat);

  if (!matProp.FetchMaterial(reactor_material_name, reactorMat))
  {
    std::cerr << "Material name not found in material.conf: " << reactor_material_name << std::endl;
    return;
  }
  m_AbsorberMaterial = std::make_unique<Material>(reactorMat);
}

bool Collector::CollectorIsInitialized() const { return m_ReflectiveMaterial && m_AbsorberMaterial; }

Material Collector::GetAbsorberMaterial() const
{
  if (!m_AbsorberMaterial)
    return {};
  return *m_AbsorberMaterial.get();
}

Material Collector::GetReflectiveMaterial() const
{
  if (!m_ReflectiveMaterial)
    return {};
  return *m_ReflectiveMaterial.get();
}

RayTraceResult Collector::RunAnalysis(const GeoDateTimeData &dateTime,
                                      const GeoLocationData &gLocation,
                                      const GeoWeatherData &weather,
                                      const GeoSolarRadiationData &solar_rad) const
{
  if (!m_STraceModel)
  {
    std::cerr << "Ray tracer model not initialized!\n";
    return {};
  }
  SPA_Input spa_in(dateTime, gLocation, weather);

  auto spa_data = getSunPosition(&spa_in);
  if (!spa_data)
  {
    std::cerr << "Invalid sun position data: Error: " << spa_data.errCode << std::endl;
    return {};
  }

  /* Estimate ray number based on the amount of direct radiation received */
  int ray_num = solar_rad.DNI * RAY_NUM_MAX / SOLAR_CONSTANT;

  if (ray_num <= 0)
    return {};

  return m_STraceModel->RunAnalysis(spa_data.azimuth,
                                    spa_data.elevation,
                                    ray_num,
                                    gLocation.Latitude,
                                    dateTime.GetDecimalYear(),
                                    dateTime.dt.time.hour);
}

/*************************************************************** End Collector() *********************************************************************/

/*************************************************************** ParabolicDish() Definition *********************************************************************/
ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                             const double &reactor_length) : Collector(dish_material_name, reactor_material_name)
{
  if (dish_diameter <= 0.0 || dish_depth <= 0.0 || reactor_width <= 0.0)
  {
    throw std::invalid_argument("Need dimensions to be positive number");
  }

  m_DishDimension = std::make_unique<Parabola>(dish_diameter, dish_depth);

  reactor_length == 0 ? m_ReactorDimension = std::make_unique<Parabola>(reactor_width)
                      : m_ReactorDimension = std::make_unique<FlatSurface>(reactor_width, reactor_length);

  reactor_length == 0 ? m_ReceiverDiameter = reactor_width : (m_ReceiverDiameter = pow(((pow(reactor_length, 2) + pow(reactor_width, 2))), 0.5));

  // Initialize Collector losses
  if (m_ReactorDimension)
  {
    m_ConvectiveLoss = std::make_unique<ConvectiveLoss>(m_ReactorDimension->GetArea());
  }

  // Initialize SolTrace Model
  auto dim = dynamic_cast<Parabola *>(m_DishDimension.get());
  if (!dim)
    throw std::runtime_error("Downcasting Failed\n");

  std::vector<CollectorSpecs> specs = {
      CollectorSpecs("PabolarDish", dish_diameter, dish_diameter, m_ReflectiveMaterial->reflectivity, m_ReflectiveMaterial->absorptivity, 0.0, dim->GetFocalLength()),
      CollectorSpecs("ReactorPlate", reactor_width, reactor_length, m_AbsorberMaterial->reflectivity, m_AbsorberMaterial->absorptivity, 0.0, 0.0)};
  m_STraceModel = std::make_unique<SolTraceModel>(specs);
}

ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width)
    : ParabolicDish(dish_material_name, reactor_material_name, dish_diameter, dish_depth, reactor_width, 0.0)
{
}

bool ParabolicDish::IsInitialized() const
{
  return (m_STraceModel && m_DishDimension && m_ReactorDimension && m_ConvectiveLoss && Collector::CollectorIsInitialized());
}

/*************************************************************** End ParabolicDish() *********************************************************************/

/*************************************************************** FlatPlate() Definition *********************************************************************/
FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name,
                     const double &surface_width, const double &surface_length, const double &absorber_width,
                     const double &absorber_length) : Collector(surface_material_name, absorber_material_name)
{
  m_SurfaceDimension = std::make_unique<FlatSurface>(surface_width, surface_length);
  m_AbsorberDimension = std::make_unique<FlatSurface>(absorber_width, absorber_length);

  // Initialize Collector losses
  if (m_AbsorberDimension)
  {
    m_ConvectiveLoss = std::make_unique<ConvectiveLoss>(m_AbsorberDimension->GetArea());

    if (m_AbsorberMaterial)
      m_ConductionLoss = std::make_unique<ConductionLoss>(m_AbsorberMaterial->thermal_conductivity,
                                                          m_AbsorberMaterial->thickness,
                                                          m_AbsorberDimension->GetArea());
  }

  // Initialize SolTrace Model
  std::vector<CollectorSpecs> specs = {
      CollectorSpecs("GlassSurface", surface_width, surface_length, m_ReflectiveMaterial->reflectivity, m_ReflectiveMaterial->absorptivity, 0.0, 0.0),
      CollectorSpecs("AbsorberPlate", absorber_width, absorber_length, m_AbsorberMaterial->reflectivity, m_AbsorberMaterial->absorptivity, 0.0, 0.0)};
  m_STraceModel = std::make_unique<SolTraceModel>(specs);
}

FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name,
                     const double &surface_width, const double &surface_length)
    : FlatPlate(surface_material_name, absorber_material_name, surface_width, surface_length,
                surface_width, surface_length) {}

bool FlatPlate::IsInitialized() const
{
  return (m_STraceModel && m_SurfaceDimension && m_AbsorberDimension && m_ConductionLoss && m_ConvectiveLoss && Collector::CollectorIsInitialized());
}