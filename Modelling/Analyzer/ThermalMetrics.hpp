#pragma once
#include <cmath>
#include <iostream>

constexpr double STEFAN_BOLTZMANN = 5.670374419e-8; // W/m²K⁴
constexpr double KELVIN_OFFSET = 273.15;

// Empirical convection coefficient [W/(m²·°C)]
inline double hcW(double windspeed)
{
  // Valid for 2 ≤ windspeed ≤ 20 m/s
  return 1.16 * (10.45 - windspeed + 10.0 * std::sqrt(windspeed));
}

static const double m_dot = 0.011; // kg/s
static const double cp = 4200;     // J/(kg·°C)

struct ThermalResult
{
  double operating_temperature_celsius;
  double incident_power_kw;
  double absorbed_power_kw;
  double convection_loss_kw;
  double conduction_loss_kw;
  double radiation_loss_kw;
  double total_loss_kw;
  double useful_energy_kw;
  double thermal_efficiency;
  double energy_balance_error_percent;

  ThermalResult() : operating_temperature_celsius(0), incident_power_kw(0),
                    absorbed_power_kw(0), convection_loss_kw(0), conduction_loss_kw(0),
                    radiation_loss_kw(0), total_loss_kw(0), useful_energy_kw(0),
                    thermal_efficiency(0), energy_balance_error_percent(0) {}

  void PrintMetrics() const
  {
    std::cout << "\n====== Thermal Performance Metrics ======\n"
              << "Operating Temperature: " << operating_temperature_celsius << " °C\n"
              << "Incident Power: " << incident_power_kw << " kW\n"
              << "Absorbed Power: " << absorbed_power_kw << " kW\n"
              << "Convection Loss: " << convection_loss_kw << " kW\n"
              << "Conduction Loss: " << conduction_loss_kw << " kW\n"
              << "Radiation Loss: " << radiation_loss_kw << " kW\n"
              << "Total Loss: " << total_loss_kw << " kW\n"
              << "Useful Energy Output: " << useful_energy_kw << " kW\n"
              << "Thermal Efficiency: " << thermal_efficiency << " %\n"
              << "Energy Balance Error: " << energy_balance_error_percent << " %\n";
  }
};

class ThermalCalculator
{
public:
  // Radiation loss (W)
  static double CalculateRadiationLoss(double emissivity, double area,
                                       double T_receiver_C, double T_ambient_C)
  {
    double T_r_K = T_receiver_C + KELVIN_OFFSET;
    double T_a_K = T_ambient_C + KELVIN_OFFSET;
    double T_r4 = T_r_K * T_r_K * T_r_K * T_r_K;
    double T_a4 = T_a_K * T_a_K * T_a_K * T_a_K;
    return emissivity * area * STEFAN_BOLTZMANN * (T_r4 - T_a4);
  }

  // Convection loss (W) – windspeed must be in [2,20] m/s, else returns 0
  static double CalculateConvectionLoss(double area, double T_receiver_C,
                                        double T_ambient_C, double windspeed)
  {
    if (windspeed < 2.0 || windspeed > 20.0)
      return 0.0;
    double h = hcW(windspeed);
    return area * (T_receiver_C - T_ambient_C) * h;
  }

  // Conduction loss (W)
  static double CalculateConductionLoss(double thermal_conductivity, double area,
                                        double T_receiver_C, double T_ambient_C,
                                        double thickness)
  {
    if (thickness <= 0.0)
      return 0.0;
    return thermal_conductivity * area * (T_receiver_C - T_ambient_C) / thickness;
  }

  // Total heat loss (W) – convenience
  static double heatLoss(double area, double thickness, double T_receiver_C,
                         double T_ambient_C, double emissivity, double windspeed,
                         double thermal_conductivity)
  {
    double rad = CalculateRadiationLoss(emissivity, area, T_receiver_C, T_ambient_C);
    double conv = CalculateConvectionLoss(area, T_receiver_C, T_ambient_C, windspeed);
    double cond = CalculateConductionLoss(thermal_conductivity, area, T_receiver_C, T_ambient_C, thickness);
    return rad + conv + cond;
  }

  // Newton-Raphson solver for receiver temperature (°C)
  static double SolveReceiverTemperature(double P_abs, double area, double thickness,
                                         double T_ambient_C, double emissivity,
                                         double windspeed, double thermal_conductivity,
                                         double T_in_C, double tol = 1e-6, int maxIter = 100)
  {
    double T = T_in_C + 50.0; // initial guess (°C)
    double h_conv = (windspeed >= 2.0 && windspeed <= 20.0) ? hcW(windspeed) : 0.0;
    double T_amb_K = T_ambient_C + KELVIN_OFFSET;
    double d_useful = m_dot * cp; // constant derivative of useful term

    for (int i = 0; i < maxIter; ++i)
    {
      // Compute heat loss at current T
      double T_K = T + KELVIN_OFFSET;
      double T_K3 = T_K * T_K * T_K;
      double T_amb_K4 = T_amb_K * T_amb_K * T_amb_K * T_amb_K;
      double T_K4 = T_K * T_K3;

      double rad = emissivity * area * STEFAN_BOLTZMANN * (T_K4 - T_amb_K4);
      double conv = (h_conv > 0.0) ? area * (T - T_ambient_C) * h_conv : 0.0;
      double cond = (thickness > 0.0) ? thermal_conductivity * area * (T - T_ambient_C) / thickness : 0.0;
      double loss = rad + conv + cond;

      double useful = m_dot * cp * (T - T_in_C);
      double F = P_abs - useful - loss;

      // Derivative of loss with respect to T (d/dT)
      double d_rad = 4.0 * emissivity * area * STEFAN_BOLTZMANN * T_K3;
      double d_conv = (h_conv > 0.0) ? area * h_conv : 0.0;
      double d_cond = (thickness > 0.0) ? thermal_conductivity * area / thickness : 0.0;
      double d_loss = d_rad + d_conv + d_cond;
      double dF = -d_useful - d_loss;

      double delta = F / dF;
      T -= delta;
      if (std::fabs(delta) < tol)
        return T;
    }
    std::cerr << "Warning: Newton-Raphson did not converge." << std::endl;
    return T;
  }

  // Simple radiation-only equilibrium temperature (ignores convection, conduction, useful heat)
  static double CalculateOperatingTemperature(double absorbed_power, double emissivity, double area)
  {
    if (absorbed_power <= 0.0 || emissivity <= 0.0 || area <= 0.0)
      return 0.0;
    double T_K = std::pow(absorbed_power / (emissivity * area * STEFAN_BOLTZMANN), 0.25);
    return T_K - KELVIN_OFFSET;
  }
};