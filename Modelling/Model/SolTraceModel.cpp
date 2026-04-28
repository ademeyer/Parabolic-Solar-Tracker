#include <iostream>
#include <stdexcept>
#include <cmath>
#include "SolTraceModel.h"

SolTraceModel::SolTraceModel(const CollectorSpecs &specs)
    : m_Initialized(false), m_CollectorSpecs(specs), m_FocalOffset(0.0)
{
  m_stContext = st_create_context();
  if (!m_stContext)
  {
    std::cerr << "Failed to create SolTrace context\n";
    return;
  }
}

SolTraceModel::SolTraceModel(const SolTraceModel &rhs)
    : m_Initialized(false), m_CollectorSpecs(rhs.m_CollectorSpecs), m_stContext(nullptr)
{
  m_stContext = st_create_context();
  if (!m_stContext)
  {
    std::cerr << "Failed to create SolTrace context in copy constructor\n";
    return;
  }
}

SolTraceModel::~SolTraceModel()
{
  Cleanup();
}

Point3f SolTraceModel::GetADishAimAlongZAxis() const
{
  // Return the current aim direction (should be a unit vector)
  return m_CollectorSpecs.aim;
}

void SolTraceModel::setupAll()
{
  if (!m_stContext)
    throw std::runtime_error("Error: SolTrace context is not initialized");

  if (!(m_CollectorSpecs.origin && m_CollectorSpecs.aim))
    throw std::runtime_error("origin or aim coordinate not set, origin:[" +
                             std::to_string(m_CollectorSpecs.origin.X) + "," +
                             std::to_string(m_CollectorSpecs.origin.Y) + "," +
                             std::to_string(m_CollectorSpecs.origin.Z) + "], aim: [" +
                             std::to_string(m_CollectorSpecs.aim.X) + "," +
                             std::to_string(m_CollectorSpecs.aim.Y) + "," +
                             std::to_string(m_CollectorSpecs.aim.Z) + "]");

  std::cout << "oring coordinate: [" << m_CollectorSpecs.origin.X << "," << m_CollectorSpecs.origin.Y << "," << m_CollectorSpecs.origin.Z << "]\n";
  std::cout << "aim direction: [" << m_CollectorSpecs.aim.X << "," << m_CollectorSpecs.aim.Y << "," << m_CollectorSpecs.aim.Z << "]\n";

  setupDish();

  setupReceiver();
}

double SolTraceModel::GetFocalLength() const
{
  return m_CollectorSpecs.focal_length;
}

void SolTraceModel::Cleanup()
{
  if (m_stContext)
  {
    st_free_context(m_stContext);
    m_stContext = nullptr;
  }
}

static bool isUnitVector(const Point3f &point)
{
  if (!point)
    return false;

  const auto &[x, y, z] = point;
  double length = std::sqrt(x * x + y * y + z * z);
  return std::abs(length - 1.0) < 1e-6;
}

void SolTraceModel::UpdateCollectorAimIfNeeded(const UpdatedAim &new_aim)
{
  const auto &sun_aim = new_aim.aim;
  if (sun_aim == m_CollectorSpecs.aim)
  {
    std::cout << "Dish aim has not changed\n";
    return;
  }

  if (!isUnitVector(sun_aim))
  {
    std::cerr << "Sun position is not a unit vector\n";
    return;
  }

  m_CollectorSpecs.aim = sun_aim;
  m_FocalOffset = new_aim.receiverOffset; // Adjust focal length based on receiver offset
  m_Initialized = false;                  // Mark as not initialized to trigger re-setup with new aim

  std::cout
      << "Updated dish aim to: [" << m_CollectorSpecs.aim.X << "," << m_CollectorSpecs.aim.Y << ","
      << m_CollectorSpecs.aim.Z << "] focal length: " << m_CollectorSpecs.focal_length
      << " reciever offset:" << m_FocalOffset << "\n";
}

void SolTraceModel::UpdateDishOriginAndAim(const Point3f &origin)
{
  if (!origin || m_CollectorSpecs.origin == origin)
    return;

  m_Initialized = false; // Mark as not initialized to trigger re-setup with new origin
  m_CollectorSpecs.origin = origin;
}

RayTraceResult SolTraceModel::RunAnalysis(const Point3f &sun_pos,
                                          int ray_num)
{
  /* Initialization is not complete until actual collector origin is set [optional] */
  if (!m_Initialized)
  {
    setupAll();
    m_Initialized = true;
  }

  ray_num = std::min(ray_num, RAY_NUM_MAX);

  std::cout << "ray num: " << ray_num << std::endl;

  setupSun(sun_pos);

  /* Set simulation parameters */
  st_sim_params(m_stContext, ray_num, 1000 * RAY_NUM_MAX, 0); // nrays, maxrays, pointfocus
  st_sim_errors(m_stContext, 1, 1);                           // sunshape on, errors on

  // Run simulation
  int result_code = st_sim_run(m_stContext, 42, nullptr, nullptr); // seed = 42
  RayTraceResult results;
  if (result_code >= 0)
  {
    results.SunXYZ = sun_pos;
    results.RayCount = ray_num;
    processResult(results);
  }
  else
  {
    std::cerr << "Ray tracing failed with code: " << result_code << std::endl;
    // Print error messages
    for (int j = 0; j < st_num_messages(m_stContext); j++)
    {
      std::cerr << "Error: " << st_message(m_stContext, j) << std::endl;
    }
  }
  return results;
}

/***************************** Private Member Functions **********************************/
void SolTraceModel::processResult(RayTraceResult &results)
{
  // Get number of intersections
  int Length = st_num_intersections(m_stContext);

  if (Length <= 0)
  {
    std::cerr << "No intersections found" << std::endl;
    return;
  }

  double SunXMin, SunXMax, SunYMin, SunYMax;
  int SunRayCount;

  // tempoary allocation
  double *Xi = new double[Length];
  double *Yi = new double[Length];
  double *Zi = new double[Length];
  double *Xc = new double[Length];
  double *Yc = new double[Length];
  double *Zc = new double[Length];
  int *Em = new int[Length];
  int *Sm = new int[Length];
  int *Rn = new int[Length];

  st_sun_stats(m_stContext, &SunXMin, &SunXMax, &SunYMin, &SunYMax, &SunRayCount);

  results.SunMax = Point2f(SunXMax, SunYMax);
  results.SunMin = Point2f(SunXMin, SunYMin);

  results.Length = Length;
  results.SunRayCount = SunRayCount;

  st_locations(m_stContext, Xi, Yi, Zi);
  st_cosines(m_stContext, Xc, Yc, Zc);
  st_elementmap(m_stContext, Em);
  st_stagemap(m_stContext, Sm);
  st_raynumbers(m_stContext, Rn);

  auto &flxmp = results.FluxMap;

  for (int i = 0; i < Length; i++)
  {
    if (Em[i] < 0 || Sm[i] < 0)
      continue; // Skip rays that did not hit any element or stage

    flxmp[Sm[i]].push_back(Ray(Xi[i], Yi[i], Zi[i],
                               Xc[i], Yc[i], Zc[i],
                               Em[i], Rn[i]));
  }

  std::cout
      << "Total intersections: " << Length << std::endl;
  std::cout << "Number of stages in context: " << flxmp.size() << std::endl;
  std::cout << "=== STAGE MAP DISTRIBUTION ===" << std::endl;

  for (const auto &[stage, map] : flxmp)
  {
    std::cout << "stage " << stage << ": Hits: " << map.size() << std::endl;
  }
  std::cout << "==============================" << std::endl;

  // Cleanup
  delete[] Xi;
  delete[] Yi;
  delete[] Zi;

  delete[] Xc;
  delete[] Yc;
  delete[] Zc;

  delete[] Em;
  delete[] Sm;
  delete[] Rn;
}

void SolTraceModel::setupSun(const Point3f &sun_pos)
{
  /* Configure sun () */
  st_sun(m_stContext, 0, 'p', 0.002327);
  /* Set up sun direction */
  st_sun_xyz(m_stContext, sun_pos.X, sun_pos.Y, sun_pos.Z);
}

void SolTraceModel::setupDish()
{
  // Surface parameters for parabolic: [focus, x0, y0, z0, x1, y1, z1, unused]
  double surface_params[8] = {
      m_CollectorSpecs.focal_length,                                                   // focal length
      m_CollectorSpecs.origin.X, m_CollectorSpecs.origin.Y, m_CollectorSpecs.origin.Z, // x0, y0, z0
      m_CollectorSpecs.aim.X, m_CollectorSpecs.aim.Y, m_CollectorSpecs.aim.Z,          // x1, y1, z1 (axis direction)
      0.0                                                                              // unused
  };

  setupCollector(m_CollectorSpecs.dish.optics_name,
                 surface_params,
                 m_CollectorSpecs.dish.width == m_CollectorSpecs.dish.height ? 'c' : 'r',
                 m_CollectorSpecs.focal_length == 0 ? 'f' : 'p',
                 m_CollectorSpecs.origin, // origin
                 m_CollectorSpecs.aim,    //  aim
                 2,                       // 1=refract, 2=reflect
                 m_CollectorSpecs.dish.width,
                 m_CollectorSpecs.dish.height,
                 m_DishStageID);

  /* dish optics */
  setupOptics(m_CollectorSpecs.dish.optics_name,
              0,
              m_CollectorSpecs.dish.reflectivity,
              m_CollectorSpecs.dish.transmitivity,
              m_CollectorSpecs.dish.slope_error,
              -1.0,
              0.0);
}

void SolTraceModel::setupReceiver()
{
  // Surface parameters for flat surface: [empty]
  double surface_params[8] = {0};
  std::cout << "focal lenth: " << m_CollectorSpecs.focal_length << ", focal offset: " << m_FocalOffset << std::endl;

  /* Deduce receiver aim&origin coordinate from collector */
  Point3f receiver_origin = m_CollectorSpecs.origin + (m_CollectorSpecs.aim * (m_CollectorSpecs.focal_length + m_FocalOffset)); // position receiver at focal length distance along the dish aim direction
  Point3f receiver_aim = Point3f(-m_CollectorSpecs.aim.X, -m_CollectorSpecs.aim.Y, -m_CollectorSpecs.aim.Z);                    // aim towards the dish (normal direction)
  std::cout << "Receiver origin: [" << receiver_origin.X << "," << receiver_origin.Y << "," << receiver_origin.Z << "]\n";
  std::cout << "Receiver aim: [" << receiver_aim.X << "," << receiver_aim.Y << "," << receiver_aim.Z << "]\n";

  setupCollector(m_CollectorSpecs.receiver.optics_name,
                 surface_params,
                 m_CollectorSpecs.receiver.width == m_CollectorSpecs.receiver.height ? 'c' : 'r',
                 m_CollectorSpecs.focal_length == 0 ? 'f' : 'p',
                 receiver_origin, // origin
                 receiver_aim,    // aim
                 1,               // 1=refract, 2=reflect
                 m_CollectorSpecs.receiver.width,
                 m_CollectorSpecs.receiver.height,
                 m_ReceiverStageID);

  /* reciever optics */
  setupOptics(m_CollectorSpecs.receiver.optics_name,
              0,
              m_CollectorSpecs.receiver.reflectivity,
              m_CollectorSpecs.receiver.transmitivity,
              m_CollectorSpecs.receiver.slope_error,
              1.0,
              0.0);
}

void SolTraceModel::setupOptics(const std::string &surface_name,
                                const int &optical_surface_num,
                                const double &relectivity,
                                const double &transmitivity,
                                const double &slope_error,
                                const double &rreal,
                                const double &rimag)
{
  /* Create optics for front surface of dish */
  int optic_id = st_add_optic(m_stContext, surface_name.c_str());
  double grating[4] = {0.0, 0.0, 0.0, 0.0};

  st_optic(
      m_stContext,
      optic_id, 1,
      'g',                    // Error distribution: gaussian
      optical_surface_num,    // Optical surface number
      0,                      // Aperture stop or grating type
      0,                      // Diffraction order
      rreal,                  // Refraction index real
      rimag,                  // Refraction index imag
      relectivity,            // Reflectivity
      transmitivity,          // Transmissivity
      grating,                // Grating coefficients
      slope_error,            // RMS slope error
      1.00,                   // RMS specularity
      0, 0, nullptr, nullptr, // No reflectivity table
      0, 0, nullptr, nullptr  // No transmissivity table
  );

  // Back surface (not used for mirror)
  st_optic(m_stContext, optic_id, 2, 'g',
           0, 0, 0, 0.0, 0.0, 0.0, 0.0,
           grating, 0.0, 0.0,
           0, 0, nullptr, nullptr,
           0, 0, nullptr, nullptr);
}

void SolTraceModel::setupCollector(const std::string &optics_name,
                                   double surface_params[],
                                   const char &aperture_type,
                                   const char &surface_type,
                                   const Point3f origin_xyz,
                                   const Point3f aim_xyz,
                                   const int &element_interaction,
                                   const double &width,
                                   const double &height,
                                   int &stage_id)
{
  // Implementation to setup collector in SolTrace using m_stContext

  /* Create stage id for Collector, if not already created (parabolic dish or flatplate ) */
  if (stage_id == -1)
    stage_id = st_add_stage(m_stContext);

  if (stage_id < 0)
    throw std::runtime_error("Failed to add stage for collector");

  /* Stage properties - aiming stage that tracks the sun */
  st_stage_flags(m_stContext, stage_id, 0, 1, 0);                                // not virtual, multi-hit enabled, not trace-through
  st_stage_xyz(m_stContext, stage_id, origin_xyz.X, origin_xyz.Y, origin_xyz.Z); // position at origin
  st_stage_aim(m_stContext, stage_id, aim_xyz.X, aim_xyz.Y, aim_xyz.Z);          // Aim at focal length
  st_stage_zrot(m_stContext, stage_id, 0.0);

  /* Create a element (parabolic or flatplate) */
  if (m_ElementID == -1)
    m_ElementID = st_add_element(m_stContext, stage_id);

  std::cout << "stage id: " << stage_id << ", element_id: " << m_ElementID << std::endl;
  if (m_ElementID < 0)
    throw std::runtime_error("Failed to add element for collector");

  /* Setup Element properties */
  st_element_enabled(m_stContext, stage_id, m_ElementID, 1);

  st_element_xyz(m_stContext, stage_id, m_ElementID, origin_xyz.X, origin_xyz.Y, origin_xyz.Z);
  st_element_aim(m_stContext, stage_id, m_ElementID, aim_xyz.X, aim_xyz.Y, aim_xyz.Z);
  st_element_zrot(m_stContext, stage_id, m_ElementID, 0.0);

  /* Set aperture type */
  st_element_aperture(m_stContext, stage_id, m_ElementID, aperture_type);

  /* Setup aperture params */
  double aperture_params[8] = {
      width,                                    // width
      height,                                   // height
      origin_xyz.X, origin_xyz.Y, origin_xyz.Z, // x0, y0, z0
      aim_xyz.X, aim_xyz.Y, aim_xyz.Z           // x1, y1, z1 (normal vector)
  };
  st_element_aperture_params(m_stContext, stage_id, m_ElementID, aperture_params);

  /* Setup Surface element */
  st_element_surface(m_stContext, stage_id, m_ElementID, surface_type);

  st_element_surface_params(m_stContext, stage_id, m_ElementID, surface_params);

  // Assign optics
  st_element_optic(m_stContext, stage_id, m_ElementID, optics_name.c_str());
  st_element_interaction(m_stContext, stage_id, m_ElementID, element_interaction);
}