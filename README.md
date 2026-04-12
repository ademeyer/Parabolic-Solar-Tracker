# Solar Collector Simulation

This repository implements a modular C++ codebase for solar energy simulation, data collection, and analysis. It combines solar collector physics, ray tracing, solar position algorithms, database support, and example applications.

The design is intended for research, prototype development, and engineering evaluation of solar collectors and tracking systems.

---

## What This Project Contains

- `Collector/`
  - Core collector library implementation.
  - `Collector.cpp` and `Collector.hpp` define thermal and optical models, material handling, and loss estimation.

- `Modelling/`
  - Ray tracing and simulation support.
  - Contains `Model/`, `Renderer/`, and `Analyzer/` modules to build and analyze flux and thermal behavior.

- `Tracking-Algorithm/`
  - Solar position, magnetic declination, and tracking algorithms.
  - Includes SPA and WMM support used by the collector and analysis tools.

- `Include/`
  - Shared public headers, external library integrations, and helper utilities.
  - Contains third-party dependencies like SolTrace and plotting connector headers.

- `Data-Service/`
  - API and sensor interface primitives.
  - Provides `Data-API/` manager classes and abstract sensor interfaces for future integration.

- `Database-Service/`
  - SQLite-based database manager implementation.
  - Used by analysis applications to persist simulation results and configuration data.

- `src/`
  - Executable applications and integration tests.
  - Includes `db-app`, `api-test`, `mock-sensor`, `plate-test`, and `analysis-runner`.

- `Configs/`
  - Configuration files for materials, simulation runs, and runtime settings.

---

## Core Capabilities

### Collector Simulation

The collector library models:

- Parabolic dish collectors
- Flat plate collector concepts
- Solar energy interception and optical efficiency
- Conduction and convection heat losses
- Operating temperature and material limits
- Incidence angle modifier effects

The collector implementation is built for extensibility, with material properties, geometry, and loss models separated into reusable components.

### Ray Tracing and Flux Modeling

The `Modelling/` module provides:

- SolTrace-based flux simulation support
- Geometry and stage management for optical models
- Renderer and analyzer support for simulation results
- Tools to convert ray tracing output into thermal input for collectors

### Solar Position and Tracking

The `Tracking-Algorithm/` module provides solar and magnetic models, including:

- Solar position algorithm (SPA)
- World Magnetic Model (WMM) support
- Tracking and orientation utilities for collector pointing

### Data and Database Support

- `Data-Service/` defines data API and sensor interfaces for simulated or real inputs.
- `Database-Service/` provides a static SQLite manager library used by applications such as the analysis runner and DB app.

---

## Build Configuration

The top-level `CMakeLists.txt` controls optional targets through flags:

- `BUILD_SENSOR_APP` — build the mock sensor application
- `BUILD_API_APP` — build the API test application
- `BUILD_DB_APP` — build the database application and Database-Service
- `BUILD_PLATE_TEST` — build the collector plate test application
- `BUILD_RUNNER_APP` — build the analysis runner tool

The collector and modelling libraries are built when `BUILD_PLATE_TEST` or `BUILD_RUNNER_APP` is enabled.

### Recommended Build Steps

```sh
cd /home/laplace/phd-project
mkdir -p build
cd build
cmake ..
cmake --build .
```

To enable specific applications:

```sh
cmake -DBUILD_PLATE_TEST=ON -DBUILD_DB_APP=ON -DBUILD_RUNNER_APP=ON ..
cmake --build .
```

If you need API or sensor applications:

```sh
cmake -DBUILD_API_APP=ON -DBUILD_SENSOR_APP=ON ..
cmake --build .
```

---

## Main Applications

### `src/plate-test/`

A test executable for the collector library:

- Builds `plate_test`
- Links against `collector`
- Copies `Configs/material.conf` into the build folder
- Uses Python integration for plotting and analysis

### `src/analysis-runner/`

A higher-level analysis application:

- Builds `analysis_runner`
- Links against `collector` and `solar_dbmanager`
- Copies `Configs/material.conf` and `Configs/run.conf`
- Copies magnetic model file `WMM.COF`
- Designed to run simulations and persist results to SQLite

### `src/db-app/`

Database-oriented executables:

- Builds `db_app` and `populate_db`
- Links against the solar database manager, API manager, and tracking libraries
- Copies `Configs/sim.conf` and required `WMM.COF`

### `src/api-test/` and `src/mock-sensor/`

Optional applications for API and sensor testing, currently enabled with the corresponding CMake flags.

---

## Configurations

Key configuration files:

- `Configs/material.conf` — material optical, thermal, and operating limits
- `Configs/run.conf` — runtime and simulation parameters for analysis tools
- `Configs/sim.conf` — database simulation configuration

### Material Configuration Format

Material definitions use comma-delimited values and describe optical and thermal performance. Example:

```ini
# Format: name=melting_point,max_operating_temp,reflectance,transmittance,absorbance,thermal_conductivity(W/m·K),material_thickness(meter)
Aluminum-Polished=933.0,673.0,0.94,0.10,0.04,237.0,0.012
Copper-Polished=1358.0,773.0,0.775,0.225,0.02,401.0,0.012
```

Update this file to add new materials or adjust optical/thermal properties.

---

## Usage

1. Build the selected applications using CMake.
2. Run `plate_test`, `analysis_runner`, or `db_app` from the build directory.
3. Provide correct configuration files in the executable working directory.
4. Inspect output logs, data files, or SQLite databases produced by the applications.

For specific command-line arguments, consult the source files in each `src/*` folder.

---

## Extending the Codebase

- Add new collector models by extending the collector classes in `Collector/`.
- Add ray tracing stages or converters in `Modelling/`.
- Add new data sources or APIs in `Data-Service/`.
- Add database persistence logic in `Database-Service/`.
- Add new apps in `src/` and enable them with the top-level CMake flags.

---

## Notes

- The code is written for C++20 and requires CMake 3.12 or newer.
- The collector library uses static linking to `soltrace_model` and SPA libraries.
- Python 3.10 is required for the `plate_test` and `analysis_runner` executables.

---

## License

MIT License
