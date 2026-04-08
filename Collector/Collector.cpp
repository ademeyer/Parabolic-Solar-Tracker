#include "Collector.hpp"

/*************************************************************** Collector() Definition *********************************************************************/
// WGS84 ellipsoid constants
static const double WGS84_A = 6378137.0;         // semi-major axis [m]
static const double WGS84_E2 = 6.69437999014e-3; // first eccentricity squared

/**
 * Convert geodetic coordinates (latitude, longitude, altitude) to ECEF Cartesian coordinates.
 *
 * @param lat Latitude in degrees
 * @param lon Longitude in degrees
 * @param alt Altitude above ellipsoid in meters
 * @param x Reference to output X coordinate (meters)
 * @param y Reference to output Y coordinate (meters)
 * @param z Reference to output Z coordinate (meters)
 */
static Point3f geodeticToEcef(double lat, double lon, double alt)
{
  lat = deg2rad(lat);
  lon = deg2rad(lon);
  double sinLat = std::sin(lat);
  double cosLat = std::cos(lat);
  double sinLon = std::sin(lon);
  double cosLon = std::cos(lon);

  double N = WGS84_A / std::sqrt(1.0 - WGS84_E2 * sinLat * sinLat);

  double x = (N + alt) * cosLat * cosLon;
  double y = (N + alt) * cosLat * sinLon;
  double z = (N * (1.0 - WGS84_E2) + alt) * sinLat;

  return Point3f(x, y, z);
}

static Point3f sunPosTo3DCartesian(const double &az, const double el)
{
  const auto az_rad = deg2rad(az);
  const auto el_rad = deg2rad(el);
  return Point3f(std::cos(el_rad) * std::sin(az_rad), // East
                 std::cos(el_rad) * std::cos(az_rad), // North
                 std::sin(el_rad));                   // Up /Zenith
}

static Point3f geodaticTo3DCartesian(const double &lat, const double lon)
{
  const auto lat_rad = deg2rad(lat);
  const auto lon_rad = deg2rad(lon);
  return Point3f(std::cos(lat_rad) * std::cos(lon_rad), // East
                 std::cos(lat_rad) * std::sin(lon_rad), // North
                 std::sin(lat_rad));                    // Up / Zenith
}

static void transformXYZToSolTrace(Point3f &coord)
{
  /**
   * @brief: Transformation description
   * X = East       = -x (in soltrace)
   * Y = North      = z (in soltrace)
   * Z = Up/Zenith  = y (in soltrace)
   *
   * Example:
   * Sun Az/El: 153.931°, 15.0188°
   * Your computed: [0.424435, -0.867585, 0.259136] (ENU: +E, +N, +U)
   * Expected SolTrace vector: [-0.424435, 0.259136, -0.867585]
   */
  coord = Point3f(-coord.X, coord.Z, coord.Y);
}

static Point3f setOrigin(const double &lat, const double &lon, const double &alt = 0.0)
{
  /* Get origin coordinates based in latitude and longitude of the collector */
  auto origin = geodaticTo3DCartesian(lat, lon);
  transformXYZToSolTrace(origin);
  return origin;
}

Collector::Collector(const std::string &dish_material_name,
                     const std::string &reactor_material_name,
                     const std::string &insulator_material_name,
                     const GeoLocationData &col_origin)
{
  // Initialize both dish and reactor material
  MaterialProperties matProp;
  Material dishMat, reactorMat, insulMat;
  if (!matProp.FetchMaterial(dish_material_name, dishMat))
  {
    std::cerr << "Reflector Material name not found in material.conf: " << dish_material_name << std::endl;
    return;
  }
  m_ReflectorMaterial = std::make_unique<Material>(dishMat);

  if (!matProp.FetchMaterial(reactor_material_name, reactorMat))
  {
    std::cerr << "Absorber Material name not found in material.conf: " << reactor_material_name << std::endl;
    return;
  }
  m_AbsorberMaterial = std::make_unique<Material>(reactorMat);

  if (!matProp.FetchMaterial(insulator_material_name, insulMat))
  {
    std::cerr << "Insulator Material name not found in material.conf: " << insulator_material_name << std::endl;
    return;
  }
  m_InsulatorMaterial = std::make_unique<Material>(insulMat);

  /* Initialized collector origin base on actual geo location [if provided]*/
  if (col_origin)
    m_Origin = setOrigin(col_origin.Latitude, col_origin.Longitude, col_origin.Altitude);
}

Collector::Collector() : Collector("", "", "", {}) {}

Collector::Collector(const Collector &other)
    : m_AbsorberMaterial(other.m_AbsorberMaterial ? std::make_unique<Material>(*other.m_AbsorberMaterial) : nullptr),
      m_ReflectorMaterial(other.m_ReflectorMaterial ? std::make_unique<Material>(*other.m_ReflectorMaterial) : nullptr),
      m_InsulatorMaterial(other.m_InsulatorMaterial ? std::make_unique<Material>(*other.m_InsulatorMaterial) : nullptr),
      m_Origin(other.m_Origin)
{
}

Collector &Collector::operator=(const Collector &other)
{
  if (this == &other)
    return *this;

  m_AbsorberMaterial = other.m_AbsorberMaterial ? std::make_unique<Material>(*other.m_AbsorberMaterial) : nullptr;
  m_ReflectorMaterial = other.m_ReflectorMaterial ? std::make_unique<Material>(*other.m_ReflectorMaterial) : nullptr;
  m_InsulatorMaterial = other.m_InsulatorMaterial ? std::make_unique<Material>(*other.m_InsulatorMaterial) : nullptr;
  m_Origin = other.m_Origin;

  return *this;
}

void Collector::UpdateCollectorOrigin(const GeoLocationData &col_origin)
{
  if (col_origin && m_StraceModel)
  {
    m_Origin = setOrigin(col_origin.Latitude, col_origin.Longitude, col_origin.Altitude);
    m_StraceModel->UpdateDishOrigin(m_Origin);
    std::cout << "Collector origin updated to [x,y,z]: [" << m_Origin.X << "," << m_Origin.Y << "," << m_Origin.Z << "]\n";
  }
  else
  {
    std::cerr << "Invalid GeoLocationData provided for collector origin update\n";
  }
}

bool Collector::CollectorIsInitialized() const
{
  return m_ReflectorMaterial && m_AbsorberMaterial && m_InsulatorMaterial;
}

Material Collector::GetAbsorberMaterial() const
{
  if (!m_AbsorberMaterial)
    return {};

  return *m_AbsorberMaterial.get();
}

Material Collector::GetReflectorMaterial() const
{
  if (!m_ReflectorMaterial)
    return {};

  return *m_ReflectorMaterial.get();
}

Material Collector::GetInsulatorMaterial() const
{
  if (!m_InsulatorMaterial)
    return {};

  return *m_InsulatorMaterial.get();
}

RayTraceResult Collector::RunAnalysis(const GeoDateTimeData &dateTime,
                                      const GeoLocationData &gLocation,
                                      const GeoWeatherData &weather,
                                      const GeoSolarRadiationData &solar_rad) const
{
  if (!m_StraceModel)
  {
    std::cerr << "Ray tracer model not initialized!\n";
    return {};
  }

  /* Calculate current sun position based on the geo location */
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

  auto sun_pos = sunPosTo3DCartesian(spa_data.azimuth,
                                     spa_data.elevation);
  return m_StraceModel->RunAnalysis(sun_pos, ray_num);
}

/*************************************************************** End Collector() *********************************************************************/

/*************************************************************** ParabolicDish() Definition *********************************************************************/
ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name, const std::string &insulator_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                             const double &reactor_length, const GeoLocationData &col_origin)
    : Collector(dish_material_name, reactor_material_name, insulator_material_name, col_origin)
{
  if (dish_diameter <= 0.0 || dish_depth <= 0.0 || reactor_width <= 0.0)
  {
    throw std::invalid_argument("Need dimensions to be positive number");
  }

  m_DishDimension = std::make_unique<Parabola>(dish_diameter, dish_depth);

  reactor_length == 0 ? m_ReactorDimension = std::make_unique<Parabola>(reactor_width) // circular
                      : m_ReactorDimension = std::make_unique<FlatSurface>(reactor_width, reactor_length);

  m_ReflectorMaterial->DumpInfo();
  m_AbsorberMaterial->DumpInfo();
  m_InsulatorMaterial->DumpInfo();
  // Initialize SolTrace Model
  auto dim = dynamic_cast<Parabola *>(m_DishDimension.get());
  if (!dim)
    throw std::runtime_error("Downcasting Failed\n");

  CollectorSpecs specs = CollectorSpecs(
      dim->GetFocalLength(), m_Origin, Point3f(0.0, 1.0, 0.0), // Let soltrace determine the aim direction based on the sun position and collector orientation
      Specs("ParabolicDish", dish_diameter, dish_diameter, m_ReflectorMaterial->reflectivity, m_ReflectorMaterial->absorptivity, 0.0),
      Specs("ReactorPlate", reactor_width, reactor_width, m_AbsorberMaterial->reflectivity, m_AbsorberMaterial->absorptivity, 0.0)

  );

  m_StraceModel = std::make_unique<SolTraceModel>(specs);
}

ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name, const std::string &insulator_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                             const GeoLocationData &col_origin)
    : ParabolicDish(dish_material_name, reactor_material_name, insulator_material_name, dish_diameter, dish_depth, reactor_width, 0.0, col_origin)
{
}

bool ParabolicDish::IsInitialized() const
{
  return (m_StraceModel && m_DishDimension && m_ReactorDimension && Collector::CollectorIsInitialized());
}

double ParabolicDish::GetAbsorberArea() const
{
  if (!m_ReactorDimension)
    return 0.0;
  return m_ReactorDimension->GetArea();
}

double ParabolicDish::GetReflectorArea() const
{
  if (!m_DishDimension)
    return 0.0;
  return m_DishDimension->GetArea();
}

/*
 * In an ideal scenerio, the insulating surface should cover the entire
 * absorber surface. Unless something change in the assumption, should
 * return absorber area
 */

double ParabolicDish::GetInsulatorArea() const { return GetAbsorberArea(); }

/*************************************************************** End ParabolicDish() *********************************************************************/

/*************************************************************** FlatPlate() Definition *********************************************************************/
FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name, const std::string &insulator_material_name,
                     const double &surface_width, const double &surface_length, const double &absorber_width,
                     const double &absorber_length, const GeoLocationData &col_origin)
    : Collector(surface_material_name, absorber_material_name, insulator_material_name, col_origin)
{
  m_SurfaceDimension = std::make_unique<FlatSurface>(surface_width, surface_length);
  m_AbsorberDimension = std::make_unique<FlatSurface>(absorber_width, absorber_length);

  // Initialize SolTrace Model
  CollectorSpecs specs = CollectorSpecs(
      0.0, m_Origin, Point3f(), // Let soltrace determine the aim direction based on the sun position and collector orientation
      Specs("GlassSurface", surface_width, surface_length, m_ReflectorMaterial->reflectivity, m_ReflectorMaterial->absorptivity, 0.0),
      Specs("AbsorberPlate", absorber_width, absorber_length, m_AbsorberMaterial->reflectivity, m_AbsorberMaterial->absorptivity, 0.0));

  m_StraceModel = std::make_unique<SolTraceModel>(specs);
}

FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name, const std::string &insulator_material_name,
                     const double &surface_width, const double &surface_length, const GeoLocationData &col_origin)
    : FlatPlate(surface_material_name, absorber_material_name, insulator_material_name, surface_width, surface_length,
                surface_width, surface_length, col_origin) {}

bool FlatPlate::IsInitialized() const
{
  return (m_StraceModel && m_SurfaceDimension && m_AbsorberDimension && Collector::CollectorIsInitialized());
}

double FlatPlate::GetAbsorberArea() const
{
  if (!m_AbsorberDimension)
    return 0.0;
  return m_AbsorberDimension->GetArea();
}

double FlatPlate::GetReflectorArea() const
{
  if (!m_SurfaceDimension)
    return 0.0;
  return m_SurfaceDimension->GetArea();
}

/*
 * In an ideal scenerio, the insulating surface should cover the entire
 * absorber surface. Unless something change in the assumption, should
 * return absorber area
 */

double FlatPlate::GetInsulatorArea() const { return GetAbsorberArea(); }

/*************************************************************** CollectorWithTracking() Definition *********************************************************************/

static Point3f rotateAxisAngle(const Point3f &v, const Point3f &axis, double theta)
{
  double c = cos(theta), s = sin(theta);
  double dot_av = axis.Dot(v);      // dot(axis, v);
  Point3f cross_av = axis.Cross(v); // cross(axis, v);
  return v * c + axis * dot_av * (1 - c) + cross_av * s;
}

static UpdatedAim singleAxisEW_VertexFixed(const Point3f &sun, const Point3f &v0, const Point3f &a0)
{
  const Point3f U(0, 1, 0); // East-West axis
  Point3f S = sun.Normalized();
  Point3f A = a0.Normalized();

  Point3f A_perp = A - U * A.Dot(U);
  Point3f S_perp = S - U * S.Dot(U);

  if (A_perp.Norm() < 1e-12 || S_perp.Norm() < 1e-12)
    return {A, 0.0};

  double mag = A_perp.Norm() * S_perp.Norm();
  double cosPhi = A_perp.Dot(S_perp) / mag;
  double sinPhi = (U.Cross(A_perp)).Dot(S_perp) / mag;
  double theta = std::atan2(sinPhi, cosPhi);

  Point3f newAim = rotateAxisAngle(A, U, theta);
  return {newAim, 0.0};
}

static UpdatedAim dualAxis_VertexFixed(const Point3f &sun, const Point3f &v0, const Point3f &a0)
{
  Point3f S = sun.Normalized();
  Point3f A = a0.Normalized();
  double cosTheta = A.Dot(S);
  double theta = std::acos(std::clamp(cosTheta, -1.0, 1.0));

  if (theta < 1e-12)
  {
    return {A, 0.0};
  }

  Point3f axis = (A.Cross(S)).Normalized(); // normalize(cross(A, S));
  Point3f newAim = rotateAxisAngle(A, axis, theta);
  // Vertex remains v0
  return {newAim, 0.0};
}

static UpdatedAim triAxis_VertexFixed(const Point3f &sun, const Point3f &v0, const Point3f &a0,
                                      double focalLength, double dni, double dniRef, double gain)
{
  // 1. Dual-axis orientation
  auto newAiminfo = dualAxis_VertexFixed(sun, v0, a0);

  // 2. Compute receiver offset (e.g., based on DNI)
  double receiverOffset = gain * (dni - dniRef);

  // Effective receiver position = v0 + (focalLength + receiverOffset) * newAim
  return {newAiminfo.aim, receiverOffset};
}

CollectorWithTracking::CollectorWithTracking(const Collector &collector_, const TrackingArchitecture &track_mode)
    : Collector(Collector::NoOpTag{}), m_CollectorType(&collector_), m_Trackmode(track_mode)
{
  /* Note: We use the protected NoOpTag constructor to avoid loading empty materials.
     CollectorWithTracking is a wrapper around an existing Collector.
     The base Collector members (materials, StraceModel) belong to m_CollectorType,
     not to this instance. This allows CollectorWithTracking to delegate all
     collector-specific functionality while adding tracking capabilities. */
}

UpdatedAim CollectorWithTracking::UpdateCollectorAimForTrackingMode(
    const Point3f &sun,
    double dni, double dniRef, double gain) const
{
  UpdatedAim aimInfo;
  auto vertex = m_CollectorType->GetOrigin();
  auto initialAim = m_CollectorType->GetSolTraceModel()->GetADishAimAlongZAxis();

  switch (m_Trackmode)
  {
  case TrackingArchitecture::FIXED_TILT:
    aimInfo.aim = initialAim;
    break;
  case TrackingArchitecture::SINGLE_AXIS_EW:
    aimInfo = singleAxisEW_VertexFixed(sun, vertex, initialAim);
    break;
  case TrackingArchitecture::DUAL_AXIS:
    aimInfo = dualAxis_VertexFixed(sun, vertex, initialAim);
    break;
  case TrackingArchitecture::TRI_AXIS:
  {
    auto *model = m_CollectorType->GetSolTraceModel();
    if (!model)
    {
      std::cerr << "Wrapped collector's SolTraceModel is not initialized!\n";
      return {initialAim, 0.0};
    }
    auto focalLength = model->GetFocalLength();
    aimInfo = triAxis_VertexFixed(sun, vertex, initialAim,
                                  focalLength, dni, dniRef, gain);
  }
  break;
  }

  return aimInfo;
}

/***
 * @brief Solar tracking implementation for CollectorWithTracking
 *
 * This method manages real-time solar tracking by:
 * 1. Calculating current sun position based on geographic location and time
 * 2. Applying tracking architecture-specific corrections
 * 3. Updating collector aim via SolTraceModel::UpdateCollectorAimIfNeeded()
 * 4. Running radiosity analysis with updated collector orientation
 *
 * Tracking architectures:
 * - FIXED_TILT: Static collector angle (typically latitude-based)
 * - SINGLE_AXIS_EW: Tracks sun elevation, fixed N-S orientation
 * - DUAL_AXIS: Full 2D tracking (azimuth and elevation)
 * - TRI_AXIS: Dual-axis + adaptive focal modulation
 *
 * References:
 * - True north vs Magnetic north declination handling
 * - Earth's magnetic field variations (currently ~80° West Canada, 2026)
 * - Formula: True bearing = Magnetic bearing + Declination (sign convention: East is positive)
 */
RayTraceResult CollectorWithTracking::RunAnalysis(
    const GeoDateTimeData &dataTime,
    const GeoLocationData &gLocation,
    const GeoWeatherData &weather,
    const GeoSolarRadiationData &solar_rad) const
{
  if (!IsInitialized())
  {
    std::cerr << "CollectorWithTracking is not properly initialized!\n";
    return {};
  }

  /* Calculate current sun position based on the geographic location and time */
  SPA_Input spa_in(dataTime, gLocation, weather);
  auto spa_data = getSunPosition(&spa_in);
  if (!spa_data)
  {
    std::cerr << "Invalid sun position data: Error: " << spa_data.errCode << std::endl;
    return {};
  }

  /* Convert sun position from azimuth/elevation to 3D Cartesian coordinates (ENU system)
     E = East, N = North, U = Up/Zenith */
  auto sun_pos = sunPosTo3DCartesian(spa_data.azimuth, spa_data.elevation);
  std::cout << "Sun position (ENU) [x,y,z]: [" << sun_pos.X << "," << sun_pos.Y << "," << sun_pos.Z << "]\n";

  /* Apply tracking mode-specific aim corrections and update collector orientation
     This will internally call SolTraceModel::UpdateCollectorAimIfNeeded() which:
     - Updates reflector/dish aim and position
     - Updates receiver/absorber position and orientation
     - Updates SolTrace aperture and surface parameters */

  auto newAim = UpdateCollectorAimForTrackingMode(sun_pos,
                                                  solar_rad.DNI, SOLAR_CONSTANT, 1.0 /*gain*/
  );
  /* Estimate ray number based on the amount of direct radiation received */
  int ray_num = solar_rad.DNI * RAY_NUM_MAX / SOLAR_CONSTANT;

  if (ray_num <= 0)
    return {};

  auto *model = m_CollectorType->GetSolTraceModel();
  if (!model)
  {
    std::cerr << "Wrapped collector's SolTraceModel is not initialized!\n";
    return {};
  }

  /* Update the collector aim in the wrapped collector's SolTrace model if needed. */
  model->UpdateCollectorAimIfNeeded(newAim);

  /* Execute radiosity analysis with the updated collector orientation. */
  return model->RunAnalysis(sun_pos, ray_num);
}

bool CollectorWithTracking::IsInitialized() const
{
  /* Verify that wrapped collector is fully initialized with all required components */
  return m_CollectorType && m_CollectorType->IsInitialized();
}
/*************************************************************** End CollectorWithTracking() *********************************************************************/