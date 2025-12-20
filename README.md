# Solar Collector Simulation

This project provides a comprehensive C++ library and test suite for simulating the thermal and optical performance of solar collectors, including Parabolic Dish and Flat Plate designs. It models solar energy interception, heat losses, collector efficiency, and ray tracing using configurable material properties and advanced algorithms.

The codebase is modular, extensible, and designed for research and engineering applications in solar energy systems.

---

## Features

- **Collector Types:**  
  - Parabolic Dish Collector (PDC)  
  - Flat Plate Collector (FPC)

- **Thermal Calculations:**  
  - Solar energy interception  
  - Conduction and convection losses  
  - Operating and maximum temperature estimation  
  - Optical efficiency modeling with incidence angle modifiers

- **Ray Tracing Integration:**  
  - SolTrace-based ray tracing for accurate flux mapping  
  - Support for multiple collector stages and geometries

- **Material Configuration:**  
  - Easily add or modify materials in `Configs/material.conf`  
  - Properties include reflectance, transmittance, absorptance, emissivity, thermal conductivity, and thickness

- **Extensible Design:**  
  - Abstract base classes for geometry (`Dimension`) and collectors (`Collector`)  
  - Modular loss models (`ConvectionLoss`, `ConductionLoss`)  
  - Integration with SPA (Solar Position Algorithm) for sun position calculations

---

## Project Structure

- `Collector/`: Core library classes (e.g., `Collector.hpp`, `MaterialsProperties.hpp`)  
- `Modelling/Model/`: Ray tracing models (e.g., `SolTraceModel.h`)  
- `src/`: Application-specific executables and tests  
- `Configs/`: Configuration files (e.g., `material.conf`)  
- `include/`: Header files for external libraries (e.g., SPA, SolTrace)  

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
Aluminum-Polished=933.0, 673.0, 0.94, 0.10, 0.04, 0.00, 237.0, 0.012
Copper-Polished=1358.0, 773.0, 0.775, 0.225, 0.02, 0.00, 401.0, 0.012
...
```

### Ray Tracing

Uses SolTrace for detailed ray path simulations, providing flux maps and intercepted energy data.

---

## Building the Entire Codebase

### Prerequisites

- C++17 or newer  
- CMake 3.10+  
- Standard build tools (gcc/g++)  
- SolTrace library (for ray tracing)  
- SPA library (for solar position calculations)

### Build Steps

```sh
# Clone or navigate to the project root
git clone https://github.com/ademeyer/Parabolic-Solar-Tracker.git
git submodule update --init --recursive
cd /Parabolic-Solar-Tracker

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build all components
make
```

### Running Tests

After building, run the test applications in `src/` subfolders (see individual READMEs for details).

---

## Applications

The `src/` folder contains specific applications built on this library:  
- `src/plate-test/`: Test suite for collector thermal properties and ray tracing.

See the README in each application folder for usage instructions.

---

## Extending the Codebase

- **Adding Materials:** Edit `Configs/material.conf` with the specified format.  
- **New Collectors:** Inherit from `Collector` and implement required methods.  
- **Custom Models:** Extend `SolTraceModel` for advanced ray tracing.

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
- SolTrace Documentation  

---

For more details, see the code comments, individual application READMEs, and the `Configs/material.conf` file.