#include <algorithm>
#include "Collector.hpp"

#define R_EARTH 6371000.0 // Earth radius in meters
/*************************************************************** Collector() Definition *********************************************************************/

static Point3f sunToAimVector(const double &az, const double el)
{
  const auto az_rad = deg2rad(az);
  const auto el_rad = deg2rad(el);
  return Point3f(-std::cos(el_rad) * std::sin(az_rad),  // East: +X
                 std::sin(el_rad),                      // Up / Zenith: +Y
                 -std::cos(el_rad) * std::cos(az_rad)); // South: +Z
}

/**
 * @brief Converts latitude/longitude/altitude to geocentric Cartesian coordinates (meters).
 *        Computes local orthonormal basis vectors (East, North, Up) at the reference point.
 *        Transforms an arbitrary geocentric point into local (East, Up, South) coordinates.
 *
 * @param lat
 * @param lon
 * @param alt
 * @return Point3f
 */
static Point3f geodeticToGeocentric(double lat, double lon, double alt = 0.0)
{
  double lat_rad = deg2rad(lat);
  double lon_rad = deg2rad(lon);
  double cos_lat = std::cos(lat_rad);
  double sin_lat = std::sin(lat_rad);
  double N = R_EARTH; // simplified: no ellipsoid flattening
  double r = (N + alt) * cos_lat;
  return Point3f(r * std::cos(lon_rad),
                 r * std::sin(lon_rad),
                 (N + alt) * sin_lat);
}

/**
 * @brief Compute local orthonormal basis (East, North, Up) at a reference geodetic point
 *
 * @param ref_lat
 * @param ref_lon
 * @param east
 * @param north
 * @param up
 */
static void computeLocalBasis(double ref_lat, double ref_lon, Point3f &east, Point3f &north, Point3f &up)
{
  double lon_rad = deg2rad(ref_lon);
  // East = (-sin(lon), cos(lon), 0) – unit vector
  east = Point3f(-std::sin(lon_rad), std::cos(lon_rad), 0.0);

  // Up = normalized geocentric position of reference point
  Point3f ref_geo = geodeticToGeocentric(ref_lat, ref_lon, 0.0);
  double len = std::sqrt(ref_geo.Dot(ref_geo));
  up = Point3f(ref_geo.X / len, ref_geo.Y / len, ref_geo.Z / len);

  // North = Up × East (cross product)
  north.X = up.Y * east.Z - up.Z * east.Y;
  north.Y = up.Z * east.X - up.X * east.Z;
  north.Z = up.X * east.Y - up.Y * east.X;
}

/**
 * @brief Get the SolTrace Dish Origin From Geodatic object. Get origin coordinates based in latitude and longitude of the collector
 *
 * @param lat
 * @param lon
 * @param alt
 * @return Point3f
 */
static Point3f getSolTraceDishOriginFromGeodatic(const double &lat, const double &lon, const double &alt = 0.0)
{
  // Compute local basis
  Point3f east, north, up;
  computeLocalBasis(lat, lon, east, north, up);

  // Reference geocentric position R (origin of local frame)
  Point3f R = geodeticToGeocentric(lat, lon, alt);
  // Dish vector from reference point to point 0,0,0
  auto origin = Point3f(R.Dot(east), R.Dot(up), R.Dot(north));
  std::cout << "before scaling to [x (east), y (up), z (north)]: [" << origin.X << "," << origin.Y << "," << origin.Z << "]\n";
  /* clamp values using the earth radius for sanity:
   0 - 1.0 over R_EARTH unit scale
  */
  origin /= R_EARTH;
  /* X = East, Y = Up, Z = South */
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
    // std::cerr << "Reflector Material name not found in material.conf: " << dish_material_name << std::endl;
    return;
  }
  m_ReflectorMaterial = std::make_unique<Material>(dishMat);

  if (!matProp.FetchMaterial(reactor_material_name, reactorMat))
  {
    // std::cerr << "Absorber Material name not found in material.conf: " << reactor_material_name << std::endl;
    return;
  }
  m_AbsorberMaterial = std::make_unique<Material>(reactorMat);

  if (!matProp.FetchMaterial(insulator_material_name, insulMat))
  {
    // std::cerr << "Insulator Material name not found in material.conf: " << insulator_material_name << std::endl;
    return;
  }
  m_InsulatorMaterial = std::make_unique<Material>(insulMat);

  /* Initialized collector origin base on actual geo location [if provided]*/
  if (col_origin)
    m_Origin = getSolTraceDishOriginFromGeodatic(col_origin.Latitude, col_origin.Longitude, col_origin.Altitude);
  else
    m_Origin = Point3f(0, 0, 0);
}

Collector::Collector() : Collector("", "", "", {}) {}

Collector::Collector(const Collector &rhs)
    : m_StraceModel(rhs.m_StraceModel ? std::make_shared<SolTraceModel>(*rhs.m_StraceModel) : nullptr),
      m_AbsorberMaterial(rhs.m_AbsorberMaterial ? std::make_unique<Material>(*rhs.m_AbsorberMaterial) : nullptr),
      m_ReflectorMaterial(rhs.m_ReflectorMaterial ? std::make_unique<Material>(*rhs.m_ReflectorMaterial) : nullptr),
      m_InsulatorMaterial(rhs.m_InsulatorMaterial ? std::make_unique<Material>(*rhs.m_InsulatorMaterial) : nullptr),
      m_Origin(rhs.m_Origin) {}

void swap(Collector &lhs, Collector &rhs) noexcept
{
  using std::swap;
  swap(lhs.m_StraceModel, rhs.m_StraceModel);
  swap(lhs.m_AbsorberMaterial, rhs.m_AbsorberMaterial);
  swap(lhs.m_ReflectorMaterial, rhs.m_ReflectorMaterial);
  swap(lhs.m_InsulatorMaterial, rhs.m_InsulatorMaterial);
  swap(lhs.m_Origin, rhs.m_Origin);
}

void Collector::UpdateCollectorOrigin(const GeoLocationData &col_origin)
{
  if (col_origin && m_StraceModel)
  {
    m_Origin = getSolTraceDishOriginFromGeodatic(col_origin.Latitude, col_origin.Longitude, col_origin.Altitude);
    m_StraceModel->UpdateDishOriginAndAim(m_Origin);
    std::cout << "Collector origin updated to [x (east), y (up), z (north)]: [" << m_Origin.X << "," << m_Origin.Y << "," << m_Origin.Z << "]\n";
  }
  else
  {
    if (!col_origin)
      std::cerr << "Invalid GeoLocationData provided for collector origin update\n";
    else
      std::cerr << "Ray tracer model not initialized, cannot update collector origin\n";
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

  auto sun_pos = sunToAimVector(spa_data.azimuth,
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

  // m_ReflectorMaterial->DumpInfo();
  // m_AbsorberMaterial->DumpInfo();
  // m_InsulatorMaterial->DumpInfo();

  // Initialize SolTrace Model
  auto dim = dynamic_cast<Parabola *>(m_DishDimension.get());
  if (!dim)
    throw std::runtime_error("Downcasting Failed\n");

  CollectorSpecs specs = CollectorSpecs(
      dim->GetFocalLength(), m_Origin, Point3f(0, 0, 1), // Aim along Z-axis (north in SolTrace coordinates)
      Specs("ParabolicDish", dish_diameter, dish_diameter, m_ReflectorMaterial->reflectivity, m_ReflectorMaterial->absorptivity, 0.0),
      Specs("ReactorPlate", reactor_width, reactor_width, m_AbsorberMaterial->reflectivity, m_AbsorberMaterial->absorptivity, 0.0)

  );

  m_StraceModel = std::make_shared<SolTraceModel>(specs);
}

/*************************************************************** ParabolicDish() *********************************************************************/

ParabolicDish::ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name, const std::string &insulator_material_name,
                             const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                             const GeoLocationData &col_origin)
    : ParabolicDish(dish_material_name, reactor_material_name, insulator_material_name, dish_diameter, dish_depth, reactor_width, 0.0, col_origin) {}

ParabolicDish::ParabolicDish(const ParabolicDish &rhs)
    : Collector(rhs),
      m_DishDimension(rhs.m_DishDimension ? std::move(rhs.m_DishDimension->Clone()) : nullptr),
      m_ReactorDimension(rhs.m_ReactorDimension ? std::move(rhs.m_ReactorDimension->Clone()) : nullptr)
{
}

ParabolicDish::ParabolicDish(ParabolicDish &&other) noexcept : Collector(std::move(other)),
                                                               m_DishDimension(std::move(other.m_DishDimension)),
                                                               m_ReactorDimension(std::move(other.m_ReactorDimension)) {}

void swap(ParabolicDish &lhs, ParabolicDish &rhs) noexcept
{
  using std::swap;
  swap(static_cast<Collector &>(lhs), static_cast<Collector &>(rhs));
  swap(lhs.m_DishDimension, rhs.m_DishDimension);
  swap(lhs.m_ReactorDimension, rhs.m_ReactorDimension);
}

ParabolicDish &ParabolicDish::operator=(ParabolicDish rhs) noexcept
{
  swap(*this, rhs);
  return *this;
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
  if (surface_width <= 0.0 || surface_length <= 0.0 || absorber_width <= 0.0 || absorber_length <= 0.0)
  {
    throw std::invalid_argument("Need dimensions to be positive number");
  }

  m_SurfaceDimension = std::make_unique<FlatSurface>(surface_width, surface_length);
  m_AbsorberDimension = std::make_unique<FlatSurface>(absorber_width, absorber_length);

  // Initialize SolTrace Model
  CollectorSpecs specs = CollectorSpecs(
      0.0, m_Origin, Point3f(0, 0, 1), // Aim along Z-axis for flat plate
      Specs("GlassSurface", surface_width, surface_length, m_ReflectorMaterial->reflectivity, m_ReflectorMaterial->absorptivity, 0.0),
      Specs("AbsorberPlate", absorber_width, absorber_length, m_AbsorberMaterial->reflectivity, m_AbsorberMaterial->absorptivity, 0.0));

  m_StraceModel = std::make_shared<SolTraceModel>(specs);
}

FlatPlate::FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name, const std::string &insulator_material_name,
                     const double &surface_width, const double &surface_length, const GeoLocationData &col_origin)
    : FlatPlate(surface_material_name, absorber_material_name, insulator_material_name, surface_width, surface_length,
                surface_width, surface_length, col_origin) {}

FlatPlate::FlatPlate(const FlatPlate &rhs)
    : Collector(rhs),
      m_SurfaceDimension(m_SurfaceDimension ? std::move(rhs.m_SurfaceDimension->Clone()) : nullptr),
      m_AbsorberDimension(m_AbsorberDimension ? std::move(rhs.m_AbsorberDimension->Clone()) : nullptr) {}

FlatPlate::FlatPlate(FlatPlate &&other) noexcept
    : Collector(std::move(other)),
      m_SurfaceDimension(std::move(other.m_SurfaceDimension)),
      m_AbsorberDimension(std::move(other.m_AbsorberDimension)) {}

void swap(FlatPlate &lhs, FlatPlate &rhs) noexcept
{
  using std::swap;
  swap(static_cast<Collector &>(lhs), static_cast<Collector &>(rhs));
  swap(lhs.m_SurfaceDimension, rhs.m_SurfaceDimension);
  swap(lhs.m_AbsorberDimension, rhs.m_AbsorberDimension);
}

FlatPlate &FlatPlate::operator=(FlatPlate rhs) noexcept
{
  swap(*this, rhs);
  return *this;
}

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
  // East-West axis in SolTrace coordinates is the X axis.
  const Point3f U(1, 0, 0);
  Point3f S = sun.Normalized();
  Point3f A = a0.Normalized();

  double anglebtw = std::acos(S.Dot(A));
  std::cout << "angle between = " << anglebtw << std::endl;

  Point3f A_perp = A - U * A.Dot(U);
  Point3f S_perp = S - U * S.Dot(U);

  if (A_perp.Norm() < 1e-12 || S_perp.Norm() < 1e-12)
    return {A, 0.0};

  double mag = A_perp.Norm() * S_perp.Norm();
  double cosPhi = A_perp.Dot(S_perp) / mag;
  double sinPhi = (U.Cross(A_perp)).Dot(S_perp) / mag;
  double theta = std::atan2(sinPhi, cosPhi);

  Point3f newAim = rotateAxisAngle(A, U, theta);

  newAim = newAim * -1.0; // invert direction

  return {newAim, 0.0};
}

static UpdatedAim dualAxis_VertexFixed(const Point3f &sun, const Point3f &v0, const Point3f &a0)
{
  (void)v0;
  Point3f S = sun.Normalized();
  Point3f A = a0.Normalized();
  double cosTheta = A.Dot(S);
  double theta = std::acos(std::clamp(cosTheta, -1.0, 1.0));

  std::cout << "angle between = " << theta << std::endl;

  if (theta < 1e-12)
  {
    return {A, 0.0};
  }

  Point3f axis = (A.Cross(S)).Normalized(); // Rotation axis is perpendicular to both A and S
  Point3f newAim = rotateAxisAngle(A, axis, theta);

  newAim = newAim * -1.0; // invert direction

  // Vertex remains v0
  return {newAim, 0.0};
}

static UpdatedAim triAxis_VertexFixed(const Point3f &sun, const Point3f &v0, const Point3f &a0,
                                      double focalLength, double dni, double dniRef)
{
  // 1. Dual-axis orientation
  auto newAiminfo = dualAxis_VertexFixed(sun, v0, a0);

  // 2. Compute receiver offset (e.g., based on DNI)
  double receiverOffset = 0.0;
  auto cmp = dni / dniRef;
  if (cmp > 0.8)
  {
    receiverOffset = 0.05 * focalLength; // Move receiver slightly forward for high DNI
    std::cout << "It is more direct normal irradiance\n";
  }
  else if (cmp < 0.6)
  {
    receiverOffset = -0.05 * focalLength; // Move receiver slightly backward for low DNI
    std::cout << "It is more difused irradiance\n";
  }

  // Alternatively, could use a more continuous function:
  // receiverOffset = 0.05 * focalLength * (dni / dniRef - 1.0); // Linear adjustment
  // gain *(dni - dniRef);

  // Effective receiver position = v0 + (focalLength + receiverOffset) * newAim
  return {newAiminfo.aim, receiverOffset};
}

CollectorWithTracking::CollectorWithTracking(std::unique_ptr<Collector> collector_, const TrackingArchitecture &track_mode)
    : Collector(), m_CollectorType(std::move(collector_)), m_Trackmode(track_mode)
{
  /* Note: We use the protected empty constructor to avoid loading empty materials.
     CollectorWithTracking is a wrapper around an existing Collector.
     The base Collector members (materials, StraceModel) belong to m_CollectorType,
     not to this instance. This allows CollectorWithTracking to delegate all
     collector-specific functionality while adding tracking capabilities. */
}

CollectorWithTracking::CollectorWithTracking(const CollectorWithTracking &rhs)
    : m_CollectorType(std::move(rhs.Clone())), m_Trackmode(rhs.m_Trackmode) {}

CollectorWithTracking::CollectorWithTracking(CollectorWithTracking &&other) noexcept
    : m_CollectorType(std::move(other.Clone())),
      m_Trackmode(std::move(other.m_Trackmode)) {}

void swap(CollectorWithTracking &lhs, CollectorWithTracking &rhs) noexcept
{
  using std::swap;
  swap(static_cast<Collector &>(lhs), static_cast<Collector &>(rhs));
  swap(lhs.m_CollectorType, rhs.m_CollectorType);
  swap(lhs.m_Trackmode, rhs.m_Trackmode);
}

CollectorWithTracking &CollectorWithTracking::operator=(CollectorWithTracking rhs) noexcept
{
  swap(*this, rhs);
  return *this;
}

UpdatedAim CollectorWithTracking::UpdateCollectorAimForTrackingMode(
    const Point3f &sun,
    double dni, double dniRef) const
{
  auto model = m_CollectorType->GetSolTraceModel();
  if (!model)
  {
    std::cerr << "Wrapped collector's SolTraceModel is not initialized!\n";
    return {};
  }

  UpdatedAim aimInfo;
  auto vertex = m_CollectorType->GetOrigin();
  auto initialAim = model->GetADishAimAlongZAxis();

  switch (m_Trackmode)
  {
  case TrackingArchitecture::FIXED_TILT:
    aimInfo.aim = initialAim;
    aimInfo.receiverOffset = 0.0;
    break;
  case TrackingArchitecture::SINGLE_AXIS_EW:
    aimInfo = singleAxisEW_VertexFixed(sun, vertex, initialAim);
    break;
  case TrackingArchitecture::DUAL_AXIS:
    aimInfo = dualAxis_VertexFixed(sun, vertex, initialAim);
    break;
  case TrackingArchitecture::TRI_AXIS:
  {
    auto focalLength = model->GetFocalLength();
    aimInfo = triAxis_VertexFixed(sun, vertex, initialAim,
                                  focalLength, dni, dniRef);
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
  if (!this->IsInitialized())
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
  auto sun_pos = sunToAimVector(spa_data.azimuth, spa_data.elevation);

  std::cout << "Sun position (SolTrace) [x,y,z]: [" << sun_pos.X << "," << sun_pos.Y << "," << sun_pos.Z << "]\n";

  /* Apply tracking mode-specific aim corrections and update collector orientation
     This will internally call SolTraceModel::UpdateCollectorAimIfNeeded() which:
     - Updates reflector/dish aim and position
     - Updates receiver/absorber position and orientation
     - Updates SolTrace aperture and surface parameters */

  auto newAim = UpdateCollectorAimForTrackingMode(sun_pos,
                                                  solar_rad.DNI, solar_rad.GHI); // Pass DNI and GHI for potential use in tri-axis tracking adjustments

  /* Estimate ray number based on the amount of direct radiation received */
  int ray_num = solar_rad.DNI * RAY_NUM_MAX / SOLAR_CONSTANT;

  if (ray_num <= 0)
    return {};

  auto model = m_CollectorType->GetSolTraceModel();
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