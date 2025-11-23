# Solar Collector Simulation

This project provides a C++ library and test suite for simulating the thermal and optical performance of solar collectors, including Parabolic Dish and Flat Plate designs. It models solar energy interception, heat losses, and collector efficiency using configurable material properties.

---

## Features

- **Collector Types:**  
  - Parabolic Dish Collector  
  - Flat Plate Collector

- **Thermal Calculations:**  
  - Solar energy intercepted  
  - Conduction and convection losses  
  - Operating and maximum temperature estimation  
  - Optical efficiency modeling

- **Material Configuration:**  
  - Easily add or modify materials in `Configs/material.conf`  
  - Properties include reflectance, transmittance, absorbance, thermal conductivity, and thickness

- **Extensible Design:**  
  - Abstract base classes for geometry and collectors  
  - Modular loss models (conduction, convection)

---

## How It Works

### Collector Models

- **Parabolic Dish:**  
  Calculates intercepted solar energy based on dish and reactor geometry, material properties, and solar input.  
  Optical efficiency formula:
  ```
  η_opt = ρ * γ * τ * α * cos(θ) * IAM
  ```
  Where:
  - ρ: mirror reflectance
  - γ: intercept factor (typically 0.9–0.95)
  - τ: transmittance of receiver cover
  - α: absorptance of receiver
  - θ: incidence angle
  - IAM: incidence angle modifier

- **Flat Plate:**  
  Models direct solar absorption and heat losses through the plate, using material-specific conduction and convection equations.

### Material Properties

Materials are defined in `Configs/material.conf`:
```
# Format: name=melting_point,max_operating_temp,reflectance,transmittance,absorbance,thermal_conductivity(W/m·K),material_thickness(meter)
Aluminum=933.0, 673.0, 0.92, 0.08, 0.00, 237.0, 0.012
Copper=1358.0, 773.0, 0.70, 0.30, 0.00, 401.0, 0.012
...
```

---

## Example Usage

```cpp
#include "Collector.hpp"

GeoWeatherData weather(25.0, 1013.25, 5.0, 50.0);
GeoSolarRadiationData solar(1000.0, 45.0, 180.0, 45.0);

ParabolicDish dish("Aluminum", "Copper", 1.5, 0.3);
if (dish.IsInitialized()) {
    auto props = dish.GetCollectorThermalProperties(weather, solar, 0);
    // props contains intercepted energy, losses, efficiency, temperatures
}
```

---

## Building

### Prerequisites

- C++17 or newer
- CMake 3.10+
- Standard build tools (gcc/g++)

### Build Steps

```sh
mkdir build
cd build
cmake ..
make
```

---

## Extending Materials

To add a new material, edit `Configs/material.conf` and add a line in the format:
```
MaterialName=melting_point,max_operating_temp,reflectance,transmittance,absorbance,thermal_conductivity,material_thickness
```

---

## Contributing

- Fork the repository
- Create a feature branch
- Submit a pull request

---

## License

MIT License

---

## References

- Duffie & Beckman, *Solar Engineering of Thermal Processes*
- ASHRAE Handbook—Fundamentals

---

For more details, see the code comments and the `Configs/material.conf` file.