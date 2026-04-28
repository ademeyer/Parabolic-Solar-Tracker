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
  virtual std::unique_ptr<Dimension> Clone() const = 0;
  virtual double GetArea() const = 0;
};

class Parabola : public Dimension
{
  double m_Diameter;
  double m_Depth;

public:
  Parabola(const double &diameter, const double &depth = 0)
      : m_Diameter(diameter), m_Depth(depth) {}
  std::unique_ptr<Dimension> Clone() const override { return std::make_unique<Parabola>(*this); }
  double GetArea() const override { return M_PI * pow((m_Diameter / 2.0), 2); }
  double GetFocalLength() const { return m_Depth <= 0.0 ? 0.0 : static_cast<double>((pow(m_Diameter, 2.0)) / (16 * m_Depth)); }
};

class FlatSurface : public Dimension
{
  double m_Length;
  double m_Width;

public:
  FlatSurface(const double &length, const double &width) : m_Length(length), m_Width(width) {}
  std::unique_ptr<Dimension> Clone() const override { return std::make_unique<FlatSurface>(*this); }
  double GetArea() const override { return m_Length * m_Width; }
};

class Collector
{
public:
  Collector(const std::string &dish_material_name,
            const std::string &reactor_material_name,
            const std::string &insulator_material_name,
            const GeoLocationData &col_origin = GeoLocationData());

  Collector();
  // Copy Contructor
  Collector(const Collector &rhs);
  /* Move constructor */
  Collector(Collector &&other) noexcept = default;
  /* Copy/Swap operator assignment*/
  // Collector &operator=(Collector rhs) noexcept;

  virtual ~Collector() = default;

  virtual void UpdateCollectorOrigin(const GeoLocationData &col_origin);

  virtual Point3f GetOrigin() const { return m_Origin; }

  std::shared_ptr<SolTraceModel> GetSolTraceModel() const { return m_StraceModel; }

  virtual Material GetAbsorberMaterial() const;

  virtual Material GetReflectorMaterial() const;

  virtual Material GetInsulatorMaterial() const;

  virtual std::unique_ptr<Collector> Clone() const = 0;

  virtual bool IsInitialized() const = 0;

  virtual double GetAbsorberArea() const = 0;

  virtual double GetReflectorArea() const = 0;

  virtual double GetInsulatorArea() const = 0;

  virtual RayTraceResult RunAnalysis(
      const GeoDateTimeData &dataTime,
      const GeoLocationData &gLocation,
      const GeoWeatherData &weather,
      const GeoSolarRadiationData &solar_rad) const;

  friend void swap(Collector &lhs, Collector &rhs) noexcept;

protected:
  std::shared_ptr<SolTraceModel> m_StraceModel = nullptr;
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

  // Copy Contructor
  ParabolicDish(const ParabolicDish &rhs);
  /* Move constructor */
  ParabolicDish(ParabolicDish &&other) noexcept;
  /* Copy/Swap operator assignment*/
  ParabolicDish &operator=(ParabolicDish rhs) noexcept;

  std::unique_ptr<Collector> Clone() const override { return std::make_unique<ParabolicDish>(*this); }

  friend void swap(ParabolicDish &lhs, ParabolicDish &rhs) noexcept;

  ~ParabolicDish() override
  {
  }

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

  friend void swap(FlatPlate &lhs, FlatPlate &rhs) noexcept;

  // Copy Contructor
  FlatPlate(const FlatPlate &rhs);
  /* Move constructor */
  FlatPlate(FlatPlate &&other) noexcept;
  /* Copy/Swap operator assignment*/
  FlatPlate &operator=(FlatPlate rhs) noexcept;

  std::unique_ptr<Collector> Clone() const override { return std::make_unique<FlatPlate>(*this); }

  ~FlatPlate() override {}

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
  explicit CollectorWithTracking(std::unique_ptr<Collector> collector_, const TrackingArchitecture &track_mode);

  // Copy Contructor
  CollectorWithTracking(const CollectorWithTracking &rhs);
  /* Move constructor */
  CollectorWithTracking(CollectorWithTracking &&other) noexcept;
  /* Copy/Swap operator assignment*/
  CollectorWithTracking &operator=(CollectorWithTracking rhs) noexcept;

  ~CollectorWithTracking() override {}

  std::unique_ptr<Collector> Clone() const override { return std::make_unique<CollectorWithTracking>(*this); }

  RayTraceResult RunAnalysis(
      const GeoDateTimeData &dataTime,
      const GeoLocationData &gLocation,
      const GeoWeatherData &weather,
      const GeoSolarRadiationData &solar_rad) const override;

  bool IsInitialized() const override;

  Material GetAbsorberMaterial() const override { return m_CollectorType->GetAbsorberMaterial(); }

  Material GetReflectorMaterial() const override { return m_CollectorType->GetReflectorMaterial(); }

  Material GetInsulatorMaterial() const override { return m_CollectorType->GetInsulatorMaterial(); }

  double GetAbsorberArea() const override { return m_CollectorType->GetAbsorberArea(); }

  double GetReflectorArea() const override { return m_CollectorType->GetReflectorArea(); }

  double GetInsulatorArea() const override { return m_CollectorType->GetInsulatorArea(); }

  void UpdateCollectorOrigin(const GeoLocationData &col_origin) override { m_CollectorType->UpdateCollectorOrigin(col_origin); }

  Point3f GetOrigin() const override { return m_CollectorType->GetOrigin(); }

  friend void swap(CollectorWithTracking &lhs, CollectorWithTracking &rhs) noexcept;

protected:
  /* Calculate optimal collector aim for the given tracking mode */
  UpdatedAim UpdateCollectorAimForTrackingMode(
      const Point3f &sun,
      double dni, double dniRef) const;

private:
  std::unique_ptr<Collector> m_CollectorType = nullptr;
  TrackingArchitecture m_Trackmode;
};