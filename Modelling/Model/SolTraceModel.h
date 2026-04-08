#ifndef __SOLTRACEMODEL_H__
#define __SOLTRACEMODEL_H__

#include <vector>
#include <map>
#include <iostream>
#include "Point.h"
#include "../../Include/External/SolTrace/coretrace/stapi.h"
#define RAY_NUM_MAX 500000

/* struct to hold updated aim and receiver offset */
struct UpdatedAim
{
  Point3f aim;
  double receiverOffset;
};

/* Struct to hold Each Ray Trace information */
struct Ray
{
  int ElementMap;
  int RayNumber;
  Point3f XYZ;
  Point3f XYZcos; // XYZ = (X,Y,Z), XYZcos = (Xcos,Ycos,Zcos)
  Ray(const double &x, const double &y, const double &z,
      const double &xcos, const double &ycos, const double &zcos,
      const int &elemap, const int &raynum)
      : XYZ(Point3f(x, y, z)), XYZcos(Point3f(xcos, ycos, zcos)),
        ElementMap(elemap), RayNumber(raynum) {}
  Ray(const Point3f &xyz, const Point3f &xyzcos,
      const int &elemap, const int &raynum)
      : XYZ(xyz), XYZcos(xyzcos),
        ElementMap(elemap), RayNumber(raynum) {}
};

/* Struct to hold ray trace results for a given collector configuration */
struct RayTraceResult
{
  int RayCount;
  int SunRayCount;
  int Length;
  Point2f SunMin;
  Point2f SunMax;                          // SunMin(Xmin,Ymin), SunMax(Xmax,Ymax)
  Point3f SunXYZ;                          // Sun position (X,Y,Z)
  std::map<int, std::vector<Ray>> FluxMap; // stage_id, RayMap
};

/* Struct to hold collector specifications for SolTrace model setup */
struct Specs
{
  std::string optics_name;
  double width;
  double height;
  double reflectivity;
  double transmitivity;
  double slope_error;

  Specs(const std::string &opt_name, const double &w,
        const double &h, const double &refl, const double &trans,
        const double &sler)
      : optics_name(opt_name), width(w), height(h), reflectivity(refl),
        transmitivity(trans), slope_error(sler) {}
};

/* Struct to hold collector specifications for SolTrace model setup */
struct CollectorSpecs
{
  double focal_length;
  Point3f origin;
  Point3f aim;
  Specs dish;
  Specs receiver;
  CollectorSpecs(const double &fl, const Point3f &ori, const Point3f &aim_dir,
                 const Specs &dish_spec, const Specs &receiver_spec)
      : focal_length(fl), origin(ori), aim(aim_dir), dish(dish_spec), receiver(receiver_spec) {}
};

class SolTraceModel
{
public:
  SolTraceModel(const CollectorSpecs &specs);

  ~SolTraceModel();

  void Cleanup();

  Point3f GetADishAimAlongZAxis() const;

  void UpdateDishOrigin(const Point3f &origin);

  double GetFocalLength() const;

  void UpdateCollectorAimIfNeeded(const UpdatedAim &new_aim);

  RayTraceResult RunAnalysis(const Point3f &sun_pos,
                             int ray_num = RAY_NUM_MAX);

private:
  st_context_t m_stContext;
  bool m_Initialized;
  int m_ReceiverStageID = -1;
  int m_DishStageID = -1;
  int m_DishElementID = -1;
  int m_ReceiverElementID = -1;
  CollectorSpecs m_CollectorSpecs;

  void setupAll();

  void setupOptics(const std::string &surface_name,
                   const int &optical_surface_num,
                   const double &relectivity,
                   const double &transmitivity,
                   const double &slope_error,
                   const double &rreal,
                   const double &rimag);

  void setupCollector(const std::string &optics_name, // Name of surface reciever
                      double surface_params[],        // surface paramter (soltrace) value
                      const char &aperture_type,      // surface aperture type [circular, rectangle]
                      const char &surface_type,       // surface type [flat or parabolic ]
                      const Point3f origin_xyz,       // origin coordinate
                      const Point3f aim_xyz,          // aiming direction coordinates
                      const int &element_interaction, // 1 = reflective, 2 = absorptive
                      const double &width,            // width of the surface [same as diameter in circular]
                      const double &height,           // height of the surface [same as diameter in circular]
                      int &stage_id                   // update or use stage id if already set
  );

  void setupDish();

  void setupReceiver();

  void setupSun(const Point3f &sun_pos);

  void processResult(RayTraceResult &result);
};

#endif // __SOLTRACEMODEL_H__