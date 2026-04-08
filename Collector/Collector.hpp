#pragma once
#include <cmath>
#include <memory>
#include <iostream>
#include <stdexcept>
#include "ISensor.h"
#include "MaterialsProperties.hpp"
#include "SolTraceModel.h"
#include "SPALib.h"

#define SOLAR_CONSTANT 1361 // W/m^2

class Dimension
{
public:
  virtual ~Dimension() = default;
  virtual double GetArea() const = 0;
};

class Parabola : public Dimension
{
  double m_Diameter;
  double m_Depth;

public:
  Parabola(const double &diameter, const double &depth = 0)
      : m_Diameter(diameter), m_Depth(depth) {}
  double GetArea() const override { return M_PI * pow((m_Diameter / 2.0), 2); }
  double GetFocalLength() const { return m_Depth <= 0.0 ? 0.0 : static_cast<double>((pow(m_Diameter, 2.0)) / (16 * m_Depth)); }
};

class FlatSurface : public Dimension
{
  double m_Length;
  double m_Width;

public:
  FlatSurface(const double &length, const double &width) : m_Length(length), m_Width(width) {}
  double GetArea() const override { return m_Length * m_Width; }
};

/**
 * @brief :
 * 1. Determine the energy flux of the source: This is the
 * energy per unit area per unit time from the source, like the
 * sun. For solar energy, the solar irradiance (power per area)
 * is needed.
 * 2. Calculate the dish's aperture area: This is the area of
 * the opening of the dish, which is the area of a circle.
 * The formula is \(A=\pi r^{2}\), where \(r\) is the radius of the
 * dish.
 * 3. Find the total power incident on the dish: Multiply the
 * solar flux by the aperture area (\(P_{incident}=\text{flux}\times A\)).
 * 4. Account for optical efficiency: The dish's mirrors aren't
 * perfect and will reflect a portion of the incident energy.
 * Multiply the incident power by the dish's optical efficiency
 * (\(\eta _{opt}\)) to find the power concentrated at the focal
 * point (\(P_{focal}=P_{incident}\times \eta _{opt}\)
 *
 */

class Collector
{
public:
  Collector(const std::string &dish_material_name,
            const std::string &reactor_material_name,
            const std::string &insulator_material_name,
            const GeoLocationData &col_origin = GeoLocationData());

  Collector();

  Collector &operator=(const Collector &other); // Disable copy assignment
  Collector(const Collector &other);            // Copy constructor

  virtual ~Collector() = default;

  /* Tag struct for protected constructor use by wrapper subclasses */
  struct NoOpTag
  {
  };

  void UpdateCollectorOrigin(const GeoLocationData &col_origin);

  virtual Material GetAbsorberMaterial() const;

  virtual Material GetReflectorMaterial() const;

  virtual Material GetInsulatorMaterial() const;

  Point3f GetOrigin() const { return m_Origin; }

  virtual bool IsInitialized() const = 0;

  virtual double GetAbsorberArea() const = 0;

  virtual double GetReflectorArea() const = 0;

  virtual double GetInsulatorArea() const = 0;

  SolTraceModel *GetSolTraceModel() const { return m_StraceModel.get(); }

  virtual RayTraceResult RunAnalysis(
      const GeoDateTimeData &dataTime,
      const GeoLocationData &gLocation,
      const GeoWeatherData &weather,
      const GeoSolarRadiationData &solar_rad) const;

protected:
  /* Protected constructor for use by wrapper subclasses (e.g., CollectorWithTracking)
     that should not load or initialize materials themselves. */
  explicit Collector(NoOpTag) {}

  std::unique_ptr<SolTraceModel> m_StraceModel = nullptr;
  std::unique_ptr<Material> m_AbsorberMaterial = nullptr;
  std::unique_ptr<Material> m_ReflectorMaterial = nullptr;
  std::unique_ptr<Material> m_InsulatorMaterial = nullptr;
  Point3f m_Origin;

  bool CollectorIsInitialized() const;
};

class ParabolicDish : public Collector
{
public:
  ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name, const std::string &insulator_material_name,
                const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                const double &reactor_length, const GeoLocationData &col_origin = GeoLocationData());

  ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name, const std::string &insulator_material_name,
                const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                const GeoLocationData &col_origin = GeoLocationData());

  bool IsInitialized() const override;

  double GetAbsorberArea() const override;

  double GetReflectorArea() const override;

  double GetInsulatorArea() const override;

private:
  std::unique_ptr<Dimension> m_DishDimension = nullptr;
  std::unique_ptr<Dimension> m_ReactorDimension = nullptr;
};

class FlatPlate : public Collector
{
public:
  FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name, const std::string &insulator_material_name,
            const double &surface_width, const double &surface_length, const double &absorber_width,
            const double &absorber_length, const GeoLocationData &col_origin = GeoLocationData());

  FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name, const std::string &insulator_material_name,
            const double &surface_width, const double &surface_length,
            const GeoLocationData &col_origin = GeoLocationData());

  bool IsInitialized() const override;

  double GetAbsorberArea() const override;

  double GetReflectorArea() const override;

  double GetInsulatorArea() const override;

private:
  std::unique_ptr<Dimension> m_SurfaceDimension = nullptr;
  std::unique_ptr<Dimension> m_AbsorberDimension = nullptr;
};

/* Collector With Tracking Capabilities Declaration */
enum class TrackingArchitecture
{
  FIXED_TILT,     // Fixed at latitude
  SINGLE_AXIS_EW, // Single axis horizontal E-W
  DUAL_AXIS,      // Two degrees of freedom
  TRI_AXIS        // Three degrees with adaptive focal modulation
};

/* Using decorator strategy to add tracking capability to an
existing Collector derived class. This class wraps a concrete
Collector implementation and adds real-time solar tracking
via the SolTraceModel::UpdateCollectorAimIfNeeded() method. */
class CollectorWithTracking : public Collector
{
public:
  CollectorWithTracking(const Collector &collector_, const TrackingArchitecture &track_mode);

  RayTraceResult RunAnalysis(
      const GeoDateTimeData &dataTime,
      const GeoLocationData &gLocation,
      const GeoWeatherData &weather,
      const GeoSolarRadiationData &solar_rad) const override;

  bool IsInitialized() const override;

  double GetAbsorberArea() const override { return m_CollectorType->GetAbsorberArea(); }

  double GetReflectorArea() const override { return m_CollectorType->GetReflectorArea(); }

  double GetInsulatorArea() const override { return m_CollectorType->GetInsulatorArea(); }

protected:
  /* Calculate optimal collector aim for the given tracking mode */
  UpdatedAim UpdateCollectorAimForTrackingMode(
      const Point3f &sun,
      double dni, double dniRef, double gain) const;

private:
  const Collector *m_CollectorType = nullptr;
  TrackingArchitecture m_Trackmode;
};