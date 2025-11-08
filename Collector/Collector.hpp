#pragma once
#define STEFAN_BOLTZMANN 5.670374419e-8
#include <bits/stdc++.h>
#include "ISensor.h"
#include "MaterialsProperties.hpp"

class Dimension
{
public:
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
    return m_Area * delta_temp * hcW(windspeed);
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
      m_Thermal_conductivity * m_Area * delta_temp / m_Thickness); }
};

class Parabola : public Dimension
{
  double m_Diameter;

public:
  Parabola(const double &diameter) : m_Diameter(diameter) {}
  double GetArea() const override { return M_PI * pow((m_Diameter / 2.0), 2); }
};

class FlatSurface : public Dimension
{
  double m_Width;
  double m_Length;

public:
  FlatSurface(const double &length, const double &width) : m_Length(length), m_Width(width) {}
  double GetArea() const override { return m_Length * m_Width; }
};

struct ThermalProperties
{
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
  virtual bool IsInitialized() const = 0;
  virtual ThermalProperties GetCollectorThermalProperties(const GeoWeatherData &weather,
                                                          const GeoSolarRadiationData &solar_rad) const = 0;
};

class ParabolicDish : public Collector
{
protected:
  std::unique_ptr<Dimension> m_DishDimension = nullptr;
  std::unique_ptr<Dimension> m_ReactorDimension = nullptr;
  std::unique_ptr<Material> m_DishMaterial = nullptr;
  std::unique_ptr<Material> m_ReactorMaterial = nullptr;
  // std::unique_ptr<ConductionLoss> m_ConductionLoss;
  std::unique_ptr<ConvectiveLoss> m_ConvectiveLoss;

public:
  ParabolicDish(const std::string &dish_material_name, const std::string &reactor_material_name,
                const double &dish_diameter, const double &reactor_width, const double &reactor_length = 0)
  {
    m_DishDimension = std::make_unique<Parabola>(dish_diameter);
    reactor_length == 0 ? m_ReactorDimension = std::make_unique<Parabola>(reactor_width)
                        : m_ReactorDimension = std::make_unique<FlatSurface>(reactor_width, reactor_length);

    // Initialize both dish and reactor material
    MaterialProperties matProp;
    Material dishMat, reactorMat;
    if (!matProp.FetchMaterial(dish_material_name, dishMat))
    {
      std::cerr << "Material name not found in material.conf: " << dish_material_name << std::endl;
      return;
    }
    m_DishMaterial = std::make_unique<Material>(dishMat);

    if (!matProp.FetchMaterial(reactor_material_name, reactorMat))
    {
      std::cerr << "Material name not found in material.conf: " << reactor_material_name << std::endl;
      return;
    }
    m_ReactorMaterial = std::make_unique<Material>(reactorMat);

    // Initialize Collector losses
    if (m_ReactorDimension)
    {
      m_ConvectiveLoss = std::make_unique<ConvectiveLoss>(m_ReactorDimension->GetArea());
    }
  }

  ThermalProperties GetCollectorThermalProperties(const GeoWeatherData &weather,
                                                  const GeoSolarRadiationData &solar_rad) const override
  {
  }

  bool IsInitialized() const override
  {
    return (m_DishDimension && m_ReactorDimension && m_DishMaterial && m_ReactorMaterial && m_ConvectiveLoss);
  }
};