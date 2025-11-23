#include <iostream>
#include <stdexcept>
#include <cmath>
#include "SolTraceModel.h"

SolTraceModel::SolTraceModel(const std::vector<CollectorSpecs> &specs)
{
  m_Initialized = false;
  const auto size = specs.size();
  if (size <= 0)
    throw std::runtime_error("CollectorSpecs can not be empty");

  m_stContext = st_create_context();
  if (!m_stContext)
    throw std::runtime_error("Failed to create SolTrace context");

  if (size > 1)
  {
    auto &config = specs[0];

    setupDish(config.optics_name,
              config.width,
              config.height,
              config.focal_length,
              config.reflectivity,
              config.transmitivity,
              config.slope_error);
  }

  auto &config = size > 1 ? specs[1] : specs[0];

  setupReceiver(config.optics_name,
                config.width,
                config.height,
                config.focal_length,
                config.reflectivity,
                config.transmitivity,
                config.slope_error);
  m_Initialized = true;
}

SolTraceModel::~SolTraceModel()
{
  Cleanup();
}

void SolTraceModel::Cleanup()
{
  if (m_stContext)
  {
    st_free_context(m_stContext);
    m_stContext = nullptr;
  }
}

RayTraceResult SolTraceModel::RunAnalysis(const double &azimuth,
                                          const double &elevation,
                                          int ray_num)
{
  if (!m_Initialized)
    return {};

  ray_num = std::min(ray_num, RAY_NUM_MAX);

  std::cout << "ray num: " << ray_num << std::endl;

  /* setup the sun based on azimuth and elevation angle position  */
  setupSun(azimuth, elevation);

  /* Set simulation parameters */
  st_sim_params(m_stContext, ray_num, 10 * RAY_NUM_MAX, 0); // nrays, maxrays, pointfocus
  st_sim_errors(m_stContext, 1, 1);                         // sunshape on, errors on

  // Run simulation
  int result_code = st_sim_run(m_stContext, 42, nullptr, nullptr); // seed = 42
  RayTraceResult results;
  if (result_code >= 0)
  {
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

  { // Get number of intersections
    int Length = st_num_intersections(m_stContext);
    std::cout << "Total intersections: " << Length << std::endl;

    // Get stage info
    int num_stages = st_num_stages(m_stContext);
    std::cout << "Number of stages in context: " << num_stages << std::endl;

    // Count stage occurrences
    std::map<int, int> stage_counts;
    int *Sm = new int[Length];
    st_stagemap(m_stContext, Sm);

    for (int i = 0; i < Length; i++)
    {
      stage_counts[Sm[i]]++;
    }

    std::cout << "=== STAGE MAP DISTRIBUTION ===" << std::endl;
    for (const auto &[stage, count] : stage_counts)
    {
      std::cout << "StageMap = " << stage << ": " << count << " hits" << std::endl;
    }
    std::cout << "==============================" << std::endl;

    delete[] Sm;
  }

  // Get number of intersections
  int Length = st_num_intersections(m_stContext);

  if (Length <= 0)
  {
    std::cerr << "No intersections found" << std::endl;
    return;
  }

  double SunXMin, SunXMax, SunYMin, SunYMax;
  int SunRayCount;
  st_sun_stats(m_stContext, &SunXMin, &SunXMax, &SunYMin, &SunYMax, &SunRayCount);

  results.SunXmax = SunXMax;
  results.SunXmin = SunXMin;
  results.SunYmax = SunYMax;
  results.SunYmin = SunYMin;
  results.Length = Length;
  results.SunRayCount = SunRayCount;

  double *Xi = new double[Length];
  double *Yi = new double[Length];
  double *Zi = new double[Length];
  double *Xc = new double[Length];
  double *Yc = new double[Length];
  double *Zc = new double[Length];
  int *Em = new int[Length];
  int *Sm = new int[Length];
  int *Rn = new int[Length];

  st_locations(m_stContext, Xi, Yi, Zi);
  st_cosines(m_stContext, Xc, Yc, Zc);
  st_elementmap(m_stContext, Em);
  st_stagemap(m_stContext, Sm);
  st_raynumbers(m_stContext, Rn);

  auto &flxmp = results.FluxMap;
  flxmp.reserve(Length);

  for (size_t i = 0; i < Length; i++)
  {
    flxmp.push_back(RayMap(Xi[i], Yi[i], Zi[i],
                           Xc[i], Yc[i], Zc[i],
                           Sm[i], Em[i], Rn[i]));
  }

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

void SolTraceModel::setupSun(const double &azimuth, const double &elevation)
{
  /* Convert sun's spherical coordinates degree to radians */
  double az_rad = azimuth * M_PI / 180.0;
  double el_rad = elevation * M_PI / 180.0;

  /* Convert sun's spherical coordinates to Cartesian coordinates */
  double sun_x = std::cos(el_rad) * std::sin(az_rad);
  double sun_y = std::cos(el_rad) * std::cos(az_rad);
  double sun_z = std::sin(el_rad);

  /* Configure sun () */
  st_sun(m_stContext, 0, 'p', 0.002327);

  /* Set up sun direction */
  st_sun_xyz(m_stContext, sun_x, sun_y, sun_z);
}

void SolTraceModel::setupDish(const std::string &optics_name,
                              const double &width,
                              const double &height,
                              const double &focal_length,
                              const double &relectivity,
                              const double &transmitivity,
                              const double &slope_error)
{
  // Surface parameters for parabolic: [focus, x0, y0, z0, x1, y1, z1, unused]
  double surface_params[8] = {
      focal_length,  // focal length
      0.0, 0.0, 0.0, // x0, y0, z0
      0.0, 0.0, 1.0, // x1, y1, z1 (axis direction)
      0.0            // unused
  };

  setupCollector(optics_name,
                 surface_params,
                 'c',
                 'p',
                 0.0, 0.0, 0.0,
                 0.0, 0.0, focal_length,
                 2, // 1=refract, 2=reflect
                 width,
                 height);

  /* dish optics */
  setupOptics(optics_name,
              0,
              relectivity,
              transmitivity,
              slope_error,
              -1.0,
              0.0);
}

void SolTraceModel::setupReceiver(const std::string &optics_name,
                                  const double &width,
                                  const double &height,
                                  const double &focal_length,
                                  const double &relectivity,
                                  const double &transmitivity,
                                  const double &slope_error)
{
  // Surface parameters for flat surface: [empty]
  double surface_params[8] = {0};

  setupCollector(optics_name,
                 surface_params,
                 'c',
                 'f',
                 0.0, 0.0, focal_length,
                 0.0, 0.0, 1.0,
                 1, // 1=refract, 2=reflect
                 width,
                 height);

  /* reciever optics */
  setupOptics(optics_name,
              0,
              relectivity,
              transmitivity,
              slope_error,
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
      0.98,                   // RMS specularity
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
                                   const double &x, const double &y, const double &z,
                                   const double &ax, const double &ay, const double &az,
                                   const int &element_interaction,
                                   const double &width,
                                   const double &height)
{
  // Implementation to setup collector in SolTrace using m_stContext

  /* Create stage for Collector (parabolic dish or flatplate ) */
  int stage_id = st_add_stage(m_stContext);

  if (stage_id < 0)
    throw std::runtime_error("Failed to add stage for collector");

  auto t_az = z;
  if (surface_type == 'f')
    t_az = 0.0;
  /* Stage properties - aiming stage that tracks the sun */
  st_stage_flags(m_stContext, stage_id, 0, 1, 0);  // not virtual, multi-hit enabled, not trace-through
  st_stage_xyz(m_stContext, stage_id, x, y, z);    // position at origin
  st_stage_aim(m_stContext, stage_id, ax, ay, az); // Aim at focal length
  st_stage_zrot(m_stContext, stage_id, 0.0);

  /* Create a element (parabolic or flatplate) */
  int element_id = st_add_element(m_stContext, stage_id);

  /* Setup Element properties */
  st_element_enabled(m_stContext, stage_id, element_id, 1);

  st_element_xyz(m_stContext, stage_id, element_id, x, y, t_az);
  st_element_aim(m_stContext, stage_id, element_id, ax, ay, az);
  st_element_zrot(m_stContext, stage_id, element_id, 0.0);

  /* Set aperture type */
  st_element_aperture(m_stContext, stage_id, element_id, aperture_type);

  /* Setup aperture params */
  double aperture_params[8] = {
      width,         // width
      height,        // height
      0.0, 0.0, 0.0, // x0, y0, z0
      0.0, 0.0, 1.0  // x1, y1, z1 (normal vector)
  };
  st_element_aperture_params(m_stContext, stage_id, element_id, aperture_params);

  /* Setup Surface element */
  st_element_surface(m_stContext, stage_id, element_id, surface_type);

  st_element_surface_params(m_stContext, stage_id, element_id, surface_params);

  // Assign optics
  st_element_optic(m_stContext, stage_id, element_id, optics_name.c_str());
  st_element_interaction(m_stContext, stage_id, element_id, element_interaction);
}