#pragma once
#define STEFAN_BOLTZMANN 5.670374419e-8
#include <cmath>
#include <memory>
#include <functional>
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

class ConvectiveLoss
{
  // this is an empirical equation and can be used for velocities 2 to 20 m/s.
  // hcW = 1.16 (10.45 - v + 10 v1/2)(W/m2°C)
  std::function<double(const double &windspeed)> hcW = [](const double &windspeed)
  { return (1.16 * (10.45 - windspeed + 10 * pow(windspeed, 0.5))); };
  double m_Area;

public:
  ConvectiveLoss(const double &area) : m_Area(area) {}
  double GetThermalLossRate(const double &delta_temp, const double &windspeed) const
  {
    if (windspeed < 2.0 || windspeed > 20.0)
      throw std::domain_error("Wind speed outside valid range (2-20 m/s)");

    return m_Area * (delta_temp + 273.15) * hcW(windspeed);
  }
};

class ConductionLoss
{
  double m_Thermal_conductivity;
  double m_Thickness;
  double m_Area;

public:
  ConductionLoss(const double &ther, const double &thick, const double &area)
      : m_Thermal_conductivity(ther), m_Thickness(thick), m_Area(area) {}
  double GetThermalLossRate(const double &delta_temp) const { return static_cast<double>(
      m_Thermal_conductivity * m_Area * (delta_temp + 273.15) / m_Thickness); }
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
protected:
  std::unique_ptr<Material> m_AbsorberMaterial = nullptr;
  std::unique_ptr<Material> m_ReflectiveMaterial = nullptr;
  bool CollectorIsInitialized() const;
  Collector(const std::string &dish_material_name, const std::string &reactor_material_name);
  Material GetAbsorberMaterial() const;
  Material GetReflectiveMaterial() const;
  double temperature(const double &energy_rate, const double &emissivity, const double &area) const
  {
    /* using stefan boltzmann equation */
    return (pow((energy_rate / (emissivity * area * STEFAN_BOLTZMANN)), 0.25) - 273.15); /* in Celsius */
  }

public:
  virtual ~Collector() = default;
  virtual bool IsInitialized() const = 0;
  virtual RayTraceResult RunAnalysis(
      const GeoDateTimeData &dataTime,
      const GeoLocationData &gLocation,
      const GeoWeatherData &weather,
      const GeoSolarRadiationData &solar_rad) const = 0;
};

class ParabolicDish : public Collector
{
public:
  ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                const double &dish_diameter, const double &dish_depth, const double &reactor_width,
                const double &reactor_length);

  ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                const double &dish_diameter, const double &dish_depth, const double &reactor_width);

  RayTraceResult RunAnalysis(const GeoDateTimeData &dataTime,
                             const GeoLocationData &gLocation,
                             const GeoWeatherData &weather,
                             const GeoSolarRadiationData &solar_rad) const override;

  bool IsInitialized() const override;
  Material GetReactorMaterial() const;
  Material GetDishMaterial() const;

private:
  std::unique_ptr<SolTraceModel> m_STraceModel;
  std::unique_ptr<Dimension> m_DishDimension = nullptr;
  std::unique_ptr<Dimension> m_ReactorDimension = nullptr;
  double m_ReceiverDiameter;
  // std::unique_ptr<ConductionLoss> m_ConductionLoss;
  std::unique_ptr<ConvectiveLoss> m_ConvectiveLoss;
};

class FlatPlate : public Collector
{
public:
  FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name,
            const double &surface_width, const double &surface_length, const double &absorber_width,
            const double &absorber_length);

  FlatPlate(const std::string &surface_material_name, const std::string &absorber_material_name,
            const double &surface_width, const double &surface_length);

  RayTraceResult RunAnalysis(const GeoDateTimeData &dataTime,
                             const GeoLocationData &gLocation,
                             const GeoWeatherData &weather,
                             const GeoSolarRadiationData &solar_rad) const override;

  bool IsInitialized() const override;

private:
  std::unique_ptr<SolTraceModel> m_STraceModel;
  std::unique_ptr<Dimension> m_SurfaceDimension = nullptr;
  std::unique_ptr<Dimension> m_AbsorberDimension = nullptr;
  std::unique_ptr<ConductionLoss> m_ConductionLoss;
  std::unique_ptr<ConvectiveLoss> m_ConvectiveLoss;
};