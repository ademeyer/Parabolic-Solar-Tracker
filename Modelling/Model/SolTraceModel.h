#ifndef __SOLTRACEMODEL_H__
#define __SOLTRACEMODEL_H__
#include <vector>
#include <map>
#include "stapi.h"
#define RAY_NUM_MAX 500000

/* Struct to hold Ray Trace Result */
struct RayMap
{
  int ElementMap, RayNumber;
  double X, Y, Z, Xcos, Ycos, Zcos;
  RayMap(const double &x, const double &y, const double &z,
         const double &xcos, const double &ycos, const double &zcos,
         const int &elemap, const int &raynum)
      : X(x), Y(y), Z(z), Xcos(xcos),
        Ycos(ycos), Zcos(zcos),
        ElementMap(elemap), RayNumber(raynum) {}
};

struct RayTraceResult
{
  int SunRayCount, Length;
  double SunXmin, SunXmax, SunYmin, SunYmax;
  double Sun_x, Sun_y, Sun_z;
  std::map<int, std::vector<RayMap>> FluxMap; // stage_id, RayMap
};

struct CollectorSpecs
{
  std::string optics_name;
  double width;
  double height;
  double reflectivity;
  double transmitivity;
  double slope_error;
  double focal_length; /* optional [for concentrated collector only]*/
  CollectorSpecs(const std::string &opt_name, const double &w,
                 const double &h, const double &refl, const double &trans,
                 const double &sler, const double &fl)
      : optics_name(opt_name), width(w), height(h),
        reflectivity(refl), transmitivity(trans), slope_error(sler), focal_length(fl) {}
};

class SolTraceModel
{
private:
  st_context_t m_stContext;
  bool m_Initialized;
  void setupOptics(const std::string &surface_name,
                   const int &optical_surface_num,
                   const double &relectivity,
                   const double &transmitivity,
                   const double &slope_error,
                   const double &rreal,
                   const double &rimag);

  void setupCollector(const std::string &optics_name,                       // Name of surface reciever
                      double surface_params[],                              // surface paramter (soltrace) value
                      const char &aperture_type,                            // surface aperture type [circular, rectangle]
                      const char &surface_type,                             // surface type [flat or parabolic ]
                      const double &x, const double &y, const double &z,    // origin coordinate
                      const double &ax, const double &ay, const double &az, // aiming direction coordinates
                      const int &element_interaction,                       // 1 = reflective, 2 = absorptive
                      const double &width,                                  // width of the surface [same as diameter in circular]
                      const double &height                                  // height of the surface [same as diameter in circular]
  );

  void setupDish(const std::string &optics_name,
                 const double &width,
                 const double &height,
                 const double &focal_length,
                 const double &relectivity,
                 const double &transmitivity,
                 const double &slope_error);

  void setupReceiver(const std::string &optics_name,
                     const double &width,
                     const double &height,
                     const double &focal_length,
                     const double &relectivity,
                     const double &transmitivity,
                     const double &slope_error);

  void setupSun(const double &azimuth,
                const double &elevation);

  void processResult(const double &lat,
                     const double &day_of_year,
                     const double &hour,
                     RayTraceResult &result);

public:
  SolTraceModel(const std::vector<CollectorSpecs> &specs);
  ~SolTraceModel();
  void Cleanup();
  RayTraceResult RunAnalysis(const double &azimuth,
                             const double &elevation,
                             int ray_num,
                             const double &lat,
                             const double &day_of_year,
                             const double &hour);
};

#endif // __SOLTRACEMODEL_H__