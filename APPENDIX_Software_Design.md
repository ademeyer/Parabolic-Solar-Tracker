# APPENDIX: SOFTWARE DESIGN AND FILE INFORMATION

## A. Overview

This appendix provides comprehensive documentation of the Parabolic Solar Tracker codebase, a sophisticated C++ library and application suite designed for thermal and optical performance simulation of solar collectors. The codebase is modular, extensible, and engineered for research and industrial applications in solar energy systems.

**Project Name:** Parabolic Solar Tracker  
**Language:** C++17/C++23  
**Build System:** CMake 3.12+  
**Primary Dependencies:** SolTrace (ray tracing), SPA (solar position algorithm), WMM (world magnetic model), SQLite3, nlohmann/json

---

## B. Architectural Overview

### B.1 High-Level Architecture

The codebase follows a **layered architecture** with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────┐
│         Applications & Runners (src/)                    │
│  ├─ Analysis Runner                                      │
│  ├─ Database Application                                 │
│  ├─ Mock Sensor                                          │
│  └─ Plate Test                                           │
├─────────────────────────────────────────────────────────┤
│  Data Layer                                              │
│  ├─ Data-Service (API Management)                        │
│  └─ Database-Service (SQLite Manager)                    │
├─────────────────────────────────────────────────────────┤
│  Domain Logic Layer                                      │
│  ├─ Collector Models (Parabolic Dish, Flat Plate)       │
│  ├─ Modelling & Analysis (Ray Tracing, Thermal)         │
│  └─ Tracking Algorithm (SPA, WMM Integration)           │
├─────────────────────────────────────────────────────────┤
│  Foundation Layer (Include/)                             │
│  ├─ Common Interfaces & Abstractions                     │
│  ├─ Material Properties & Configuration                  │
│  ├─ Sensor Data Structures                               │
│  └─ External Library Headers                             │
└─────────────────────────────────────────────────────────┘
```

### B.2 Design Patterns

1. **Decorator Pattern**: `CollectorWithTracking` wraps any `Collector` implementation to add tracking capabilities without modifying the base classes.

2. **Abstract Factory Pattern**: `MaterialsParser` and configuration system provide flexible material property loading.

3. **Template Method Pattern**: Base `Collector` class defines algorithm structure; subclasses implement specific collector types.

4. **Strategy Pattern**: Different tracking architectures (`FIXED_TILT`, `SINGLE_AXIS_EW`, `DUAL_AXIS`, `TRI_AXIS`) implemented as strategies.

5. **Singleton-like Pattern**: `SolarDatabaseManager` manages database connections centrally.

---

## C. Directory Structure and Component Details

### C.1 Root Directory Files

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Master build configuration; orchestrates all subdirectories |
| `README.md` | Project overview, feature list, build instructions |
| `TRACKING_IMPLEMENTATION.md` | Detailed tracking architecture documentation |
| `APPENDIX_Software_Design.md` | This file - complete software design reference |
| `Configs/` | Configuration files (materials, simulations, runtime parameters) |
| `extract_text.py` | Python utility for text extraction from results |

### C.2 Core Component: Collector (`Collector/`)

**Files:**
- `Collector.hpp` - Core class definitions
- `Collector.cpp` - Implementation

**Key Classes:**

#### `Dimension` (Abstract Base Class)
- **Purpose**: Defines interface for geometric objects
- **Methods**: `GetArea() const` - returns aperture area
- **Subclasses**:
  - `Parabola`: Parabolic dish geometry
    - Properties: `m_Diameter`, `m_Depth`
    - Methods: `GetArea()`, `GetFocalLength()`
  - `FlatSurface`: Rectangular flat plate geometry
    - Properties: `m_Length`, `m_Width`
    - Methods: `GetArea()`

#### `Collector` (Main Domain Class)
- **Purpose**: Models a solar collector with material properties and thermal calculations
- **Key Properties**:
  - `m_DishMaterial`: Material of reflecting/absorbing surface
  - `m_ReactorMaterial`: Material of receiver/absorber
  - `m_InsulatorMaterial`: Insulation layer material
  - `m_Origin`: Geolocation (`GeoLocationData`)
- **Key Methods**:
  - `GetInterceptedEnergy()`: Calculates incident solar energy
  - `GetThermalEnergy()`: Computes thermal output considering losses
  - `GetOpticalEfficiency()`: Computes η_opt = ρ × γ × τ × α × cos(θ) × IAM
  - `RunAnalysis()`: Executes comprehensive thermal and optical analysis

#### `TrackingArchitecture` Enum
```cpp
enum class TrackingArchitecture {
  FIXED_TILT,     // Fixed at latitude (no active tracking)
  SINGLE_AXIS_EW, // Single axis horizontal E-W (elevation changes)
  DUAL_AXIS,      // Two-axis tracking (azimuth + elevation)
  TRI_AXIS        // Three-axis with focal modulation
};
```

#### `CollectorWithTracking` (Decorator)
- **Purpose**: Adds real-time tracking to any `Collector` subclass
- **Pattern**: Wraps `const Collector&` reference
- **Key Methods**:
  - `UpdateCollectorAimIfNeeded()`: Adjusts collector orientation based on sun position
  - `RunAnalysisWithTracking()`: Integrates tracking into thermal analysis

---

### C.3 Core Component: Modelling (`Modelling/`)

#### C.3.1 Analyzer Subdirectory (`Modelling/Analyzer/`)

**Purpose**: Comprehensive analysis and optimization tools for collector performance

| File | Responsibility |
|------|-----------------|
| `RunAnalysis.hpp` | Main analysis runner; orchestrates all analysis operations |
| `Chapter4ResultsAnalyzer.hpp` | Aggregates and analyzes results from Chapter 4 research |
| `SensitivityAnalysis.hpp` | Parameter sensitivity studies |
| `ParametricOptimizationPlotter.hpp` | Generates optimization plots and visualizations |
| `ResultsComparator.hpp` | Compares multiple simulation results |
| `OptimizationAndAggregation.hpp` | Aggregation logic for optimization results |
| `ThermalMetrics.hpp` | Computes thermal performance metrics |
| `ValidationFramework.hpp` | Validates simulation results against benchmarks |
| `TrackingArchitecture.hpp` | Implements tracking algorithms and strategies |

**Key Functions:**
- Parametric study generation
- Multi-scenario comparison
- Thermal efficiency calculations
- Performance visualization

#### C.3.2 Model Subdirectory (`Modelling/Model/`)

| File | Purpose |
|------|---------|
| `SolTraceModel.h` | Interface to SolTrace ray tracing engine |
| `SolTraceModel.cpp` | SolTrace integration implementation |

**Key Class: `SolTraceModel`**
- Manages ray tracing simulations
- Provides flux distribution on receiver
- Handles complex geometry descriptions
- Integrates with SolTrace external library

#### C.3.3 Renderer Subdirectory (`Modelling/Renderer/`)
- Visualization and 3D rendering components
- Geometry visualization
- Heat flux distribution rendering

---

### C.4 Foundation Layer: Include (`Include/`)

**Purpose**: Common headers, interfaces, and data structures used across the codebase

#### Core Headers

| File | Contents |
|------|----------|
| `ISensor.h` | Sensor data structures and interfaces |
| `ICompute.h` | Computational interfaces |
| `MaterialsProperties.hpp` | Material database and parser |
| `ConfigParser.hpp` | Generic configuration file parser template |
| `Point.h` | 3D point mathematics (Point3f structure) |
| `Matrix.hpp` | Matrix operations for thermal calculations |
| `HTTPHelper.hpp` | HTTP communication utilities |
| `json.hpp` | nlohmann/json library integration |

#### Data Structures Defined in `ISensor.h`

**Sensor Data:**
- `IMUSensorData`: Accelerometer, gyroscope, magnetometer readings
- `GeoLocationData`: Latitude, longitude, altitude, speed
- `GeoWeatherData`: Temperature, pressure, humidity
- `GPSSensorData`: GPS position and quality metrics

**Interfaces:**
- `ISensor`: Abstract base class for sensor implementations
- `IGPSSensor`: GPS sensor interface
- `IMUSensor`: IMU sensor interface
- `IWeather`: Weather data interface
- `IMotor`: Motor control interface

#### Material Properties System

**Structure: `Material`**
```cpp
struct Material {
  std::string name;
  double melting_point;           // Kelvin
  double max_operating_temp;      // Kelvin
  double reflectivity;            // ρ
  double absorptivity;            // α
  double emissivity;              // ε
  double transmittivity;          // τ
  double thermal_conductivity;    // W/m·K
  double thickness;               // meter
};
```

**Parser: `MaterialsParser`**
- Parses `Configs/material.conf`
- Provides validated material properties
- Handles thermal property lookups

#### External Libraries (`Include/External/`)

- `matplotlib-cpp/`: C++ interface to Python matplotlib
- `SolTrace/`: Ray tracing simulation headers
- `SPA/`: Solar Position Algorithm integration

---

### C.5 Tracking Algorithm Component (`Tracking-Algorithm/`)

**Purpose**: Solar position calculation and magnetic field modeling

| Subdirectory | Purpose |
|--------------|---------|
| `SPALibs/` | Solar Position Algorithm implementation and headers |
| `WMMLibs/` | World Magnetic Model library (WMM2025) |
| `wmm2025_Linux/` | WMM2025 Linux-specific data files |
| `solar-tracker/` | Solar tracking algorithm implementations |

**Key Functionality:**
- Calculate sun position (azimuth, elevation) for given location/time
- Compute solar declination and hour angle
- Provide magnetic field data via WMM
- Enable tracking controller decision-making

---

### C.6 Data Service Component (`Data-Service/`)

**Purpose**: API management and sensor data abstraction

#### C.6.1 Data-API Subdirectory

| File | Responsibility |
|------|-----------------|
| `APIManager.hpp/.cpp` | Central API request handler |
| `GeoLocationServiceSIM.hpp` | Simulated geolocation provider |
| `GeoWeatherServiceSIM.hpp` | Simulated weather data provider |
| `GeoDateTimeServiceSIM.hpp` | Simulated datetime/NTP service |
| `GeoSolarRadiationServiceSIM.hpp` | Simulated solar radiation data |

**Purpose**: Provides abstraction layer for sensor/API data; can switch between real and simulated data sources.

#### C.6.2 Sensor Subdirectory

| File | Purpose |
|------|---------|
| `IDateTime.h` | DateTime service interface |
| `IGPSSensor.h` | GPS sensor interface |
| `IMotor.h` | Motor control interface |
| `IMUSensor.h` | IMU sensor interface |
| `IWeather.h` | Weather data interface |

---

### C.7 Database Service Component (`Database-Service/`)

**Purpose**: Persistent storage and data management

| File | Responsibility |
|------|-----------------|
| `SolarDatabaseManager.h/.cpp` | SQLite database management |
| `database-common.hpp` | Common database structures and types |

**Key Class: `SolarDatabaseManager`**
- Database initialization and connection management
- Template-based insert operations
- Query interface for historical data
- Date-range based data retrieval

**Database Schema:**
- Location table (site coordinates and metadata)
- Performance logs (thermal output, efficiency, timestamps)
- Configuration snapshots
- Tracking data

---

### C.8 Applications and Runners (`src/`)

**Purpose**: Executable applications that demonstrate and test the codebase

| Application | Purpose |
|-------------|---------|
| `analysis-runner/` | Comprehensive analysis runner; executes parametric studies and optimizations |
| `db-app/` | Database application; manages data logging and queries |
| `api-test/` | Tests Data-Service API functionality |
| `mock-sensor/` | Generates synthetic sensor data for testing |
| `plate-test/` | Tests Flat Plate collector models |

**Key Application: `analysis-runner`**
- Orchestrates parametric studies
- Executes sensitivity analyses
- Compares multiple collector configurations
- Generates results for dissertation
- Exports data to JSON and CSV formats

---

### C.9 Configuration Files (`Configs/`)

| File | Purpose |
|------|---------|
| `material.conf` | Material property database |
| `sim.conf` | Simulation parameters (simulation days, resolution) |
| `run.conf` | Runtime configuration (locations, collector specs) |

**Example: `material.conf` Format**
```
# Name, Melting Point, Max Op Temp, Reflectance, Transmittance, Absorptance, Thermal Cond, Thickness
Aluminum-Polished=933.0, 673.0, 0.94, 0.10, 0.04, 237.0, 0.012
Copper-Polished=1358.0, 773.0, 0.775, 0.225, 0.02, 401.0, 0.012
Glass-Clear=2043.0, 673.0, 0.08, 0.85, 0.07, 1.0, 0.004
```

---

## D. Build System Architecture

### D.1 CMake Configuration

**Master CMakeLists.txt** orchestrates conditional compilation:

```cmake
set(BUILD_SENSOR_APP OFF CACHE BOOL)        # Enable mock sensor application
set(BUILD_API_APP OFF CACHE BOOL)           # Enable API testing
set(BUILD_DB_APP OFF CACHE BOOL)            # Enable database application
set(BUILD_PLATE_TEST OFF CACHE BOOL)        # Enable flat plate collector test
set(BUILD_RUNNER_APP OFF CACHE BOOL)        # Enable analysis runner tool
```

**Dependency Graph:**
```
Tracking-Algorithm (always built)
    ↓
Database-Service (if DB_APP or RUNNER_APP)
    ↓
Data-Service (if DB_APP)
    ↓
Modelling, Collector, Include (if PLATE_TEST or RUNNER_APP)
    ↓
src/ (conditional applications)
```

### D.2 Build Output Structure

```
build/
├── bin/                           # Executables
│   ├── analysis-runner            # Main analysis application
│   ├── db-app                     # Database manager
│   ├── mock-sensor                # Sensor simulator
│   └── plate-test                 # Collector test
├── lib/                           # Static/shared libraries
├── _deps/                         # External dependencies
│   ├── googletest-src/            # Unit testing framework
│   ├── nlohmann_json-src/         # JSON library
│   └── SolTrace/                  # Ray tracing engine
└── CMakeFiles/                    # Build metadata
```

---

## E. Class Hierarchy and Relationships

### E.1 Collector Class Hierarchy

```
                ┌──────────────┐
                │  Dimension   │ (Abstract)
                └──────┬───────┘
                       │
            ┌──────────┴──────────┐
            │                     │
        ┌───▼────┐         ┌──────▼──────┐
        │Parabola│         │FlatSurface  │
        └────────┘         └─────────────┘


            ┌──────────────┐
            │  Collector   │ (Abstract)
            └──────┬───────┘
                   │
        ┌──────────┴──────────┐
        │                     │
    ┌───▼─────┐       ┌──────▼──────┐
    │Parabolic│       │  FlatPlate  │
    │Dish     │       │Collector    │
    └────┬────┘       └─────────────┘
         │
         │ (wrapped by)
         │
    ┌────▼────────────────┐
    │CollectorWithTracking│ (Decorator)
    └─────────────────────┘
```

### E.2 Key Dependencies

**Collector depends on:**
- `Dimension` (geometric representation)
- `Material` (material properties)
- `SolTraceModel` (ray tracing)
- `SPALib` (solar position)
- `GeoLocationData` (site coordinates)

**Analysis Depends on:**
- `Collector` (or `CollectorWithTracking`)
- `Config` files
- Database manager (for logging)

**Database depends on:**
- SQLite3 library
- `GeoLocationData`, `GeoWeatherData` structures

---

## F. Data Flow Diagrams

### F.1 Simulation Workflow

```
Start Analysis
    ↓
Load Configuration
├─ material.conf → MaterialsParser → Material vector
├─ run.conf → ConfigParser → Site locations & specs
└─ sim.conf → Simulation parameters
    ↓
For Each Location:
    ├─ Create Collector (material selection)
    ├─ Optionally wrap with CollectorWithTracking
    └─ For Each Time Step:
        ├─ Get Solar Position (SPA calculation)
        ├─ Update Collector Aim (if tracking enabled)
        ├─ Run Ray Tracing (SolTraceModel)
        ├─ Calculate Thermal Performance
        ├─ Compute Efficiency Metrics
        └─ Log to Database
    ↓
Aggregate Results
    ├─ Parametric Studies
    ├─ Sensitivity Analysis
    └─ Performance Comparisons
    ↓
Export & Visualize
└─ JSON/CSV files, Matplotlib plots
```

### F.2 Data Access Pattern

```
Application
    ↓
Analysis Runner (RunAnalysis.hpp)
    ↓
    ├─→ Collector/CollectorWithTracking
    │       ├─→ SolTraceModel
    │       ├─→ SPALib
    │       └─→ MaterialsProperties
    ├─→ SolarDatabaseManager
    │       └─→ SQLite3
    └─→ Various Analyzers
            ├─→ RunAnalysis.hpp
            ├─→ SensitivityAnalysis.hpp
            ├─→ ParametricOptimizationPlotter.hpp
            └─→ ResultsComparator.hpp
```

---

## G. Key Algorithms and Formulas

### G.1 Optical Efficiency

$$\eta_{opt} = \rho \times \gamma \times \tau \times \alpha \times \cos(\theta) \times IAM$$

Where:
- **ρ**: Mirror reflectance (from Material)
- **γ**: Intercept factor (~0.9-0.95, geometric factor)
- **τ**: Transmittance of receiver cover (from Material)
- **α**: Absorptance of receiver (from Material)
- **θ**: Incidence angle (solar geometry)
- **IAM**: Incidence Angle Modifier (empirical or detailed model)

### G.2 Incident Power Calculation

$$P_{incident} = Solar\_Constant \times A_{aperture} \times \cos(\theta_{zenith})$$

Where:
- **Solar_Constant**: 1361 W/m² (defined in Collector.hpp)
- **A_aperture**: Aperture area from `Dimension::GetArea()`
- **θ_zenith**: Zenith angle from SPA calculation

### G.3 Thermal Power Output

$$P_{thermal} = P_{focal} - Q_{losses}$$

Where:
- **P_focal**: Power at focal point after optical losses
- **Q_losses**: Conduction losses + Convection losses

Losses calculated using:
- Conduction: $Q_{cond} = \frac{k \cdot A \cdot \Delta T}{d}$ (from Material thermal conductivity)
- Convection: $Q_{conv} = h \cdot A \cdot \Delta T$ (empirical h coefficient)

### G.4 Thermal Efficiency

$$\eta_{thermal} = \frac{P_{thermal}}{P_{incident}}$$

---

## H. External Dependencies

### H.1 Third-Party Libraries

| Library | Purpose | Type |
|---------|---------|------|
| **SolTrace** | Ray tracing simulations | External executable + library |
| **SPA (Solar Position Algorithm)** | Sun position calculation | Library (SPALibs/) |
| **WMM (World Magnetic Model)** | Magnetic field data | Library (WMMLibs/) |
| **SQLite3** | Local database | System library |
| **nlohmann/json** | JSON parsing/generation | Header-only (included) |
| **matplotlib-cpp** | Python plotting interface | C++ wrapper |
| **GoogleTest** | Unit testing framework | Test dependency |

### H.2 External Executable Integration

**SolTrace:**
- Called via subprocess for detailed ray tracing
- Reads geometry input files
- Outputs flux distribution maps
- Integrated through `SolTraceModel` class

---

## I. Configuration and Runtime Customization

### I.1 Material Configuration

Users can add custom materials by editing `Configs/material.conf`:

```
CustomMaterial=1500.0, 800.0, 0.92, 0.08, 0.05, 200.0, 0.015
```

Parser automatically validates properties and handles type conversions.

### I.2 Simulation Configuration

**sim.conf** specifies:
- Simulation start/end dates
- Time step resolution (hourly, every 15 min, etc.)
- Weather data source
- Output frequency

**run.conf** specifies:
- Collector locations (latitude, longitude, altitude)
- Collector geometry (diameter, focal length for dishes)
- Collector materials (dish, reactor, insulation)
- Tracking mode (`FIXED_TILT`, `SINGLE_AXIS_EW`, etc.)

### I.3 Runtime Parameters

CMake build options enable/disable major application components without code changes:

```bash
cmake .. -DBUILD_RUNNER_APP=ON -DBUILD_DB_APP=ON
```

---

## J. File Naming Conventions and Patterns

### J.1 Header Files

| Pattern | Convention |
|---------|-----------|
| `.hpp` | C++ header files (STL-style modern C++) |
| `.h` | C header files / legacy interfaces |
| `I*.h` | Interface/abstract base classes (e.g., `ISensor.h`, `ICompute.h`) |

### J.2 Source Files

| Pattern | Purpose |
|---------|---------|
| `.cpp` | C++ implementation files |
| `*.hpp` | Header-only implementations (templates) |

### J.3 Configuration Files

| Pattern | Purpose |
|---------|---------|
| `*.conf` | Line-based configuration (materials, simulation params) |
| `*.json` | Structured data (results, complex configs) |

### J.4 Class Naming

| Pattern | Example |
|---------|---------|
| Concrete classes | `Collector`, `Parabola`, `Material` |
| Abstract classes | `Dimension`, `Collector` (pure virtual methods) |
| Interfaces | `ISensor`, `ICompute` (pure virtual) |
| Enums | `TrackingArchitecture` |
| Structs (data) | `GeoLocationData`, `Material` |
| Managers | `SolarDatabaseManager`, `MaterialsParser` |

---

## K. Key Design Considerations

### K.1 Extensibility

The codebase is designed for easy extension:

1. **New Collector Types**: Subclass `Collector`, implement thermal model
2. **New Tracking Modes**: Add to `TrackingArchitecture` enum
3. **New Analysis Methods**: Add to `Modelling/Analyzer/` directory
4. **New Sensors**: Implement `ISensor` interface in `Data-Service/`
5. **Custom Materials**: Add entries to `material.conf`

### K.2 Separation of Concerns

- **Domain Logic** (Collector, Material): Independent of UI, storage
- **Data Access** (SolarDatabaseManager): Centralized database interaction
- **Analysis** (Analyzer/*): Pluggable analysis strategies
- **Configuration** (*.conf): External, no recompilation required

### K.3 Performance Considerations

1. **Ray Tracing**: Delegated to external SolTrace (C/Fortran for speed)
2. **Solar Position**: Pre-computed arrays where possible (SPA)
3. **Database**: Indexed queries for historical data
4. **Thermal Calculations**: Operating point optimization via parametric studies

### K.4 Validation and Testing

- **Unit Testing**: GoogleTest framework
- **Integration Testing**: `api-test/`, `plate-test/` applications
- **Validation Tools**: `ValidationFramework.hpp` for benchmark comparison
- **Result Comparison**: `ResultsComparator.hpp` for multi-scenario analysis

---

## L. Compilation and Runtime Flow

### L.1 Build Process

```bash
# Configure
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_RUNNER_APP=ON

# Compile
cmake --build . --parallel 4

# Run
./bin/analysis-runner
```

### L.2 Runtime Dependencies

At runtime, the application requires:
1. **Configuration files** in `Configs/` directory
2. **Material database** (`material.conf`)
3. **SolTrace executable** in system PATH (if ray tracing enabled)
4. **Weather data source** (file, API, or simulated)

### L.3 Output Artifacts

The applications produce:
- **JSON results**: Machine-readable simulation outputs
- **CSV exports**: Time-series data for spreadsheet analysis
- **Database logs**: SQLite database with historical data
- **Visualization plots**: Matplotlib figures (efficiency curves, flux maps)

---

## M. Integration Points with Research

### M.1 Connection to Dissertation

The codebase directly supports dissertation research by providing:

1. **Parametric Studies**: Explore design space systematically
2. **Sensitivity Analysis**: Identify key performance drivers
3. **Comparison Framework**: Validate against published data
4. **Optimization Algorithms**: Find optimal collector configurations
5. **Tracking Simulation**: Evaluate active tracking benefits

### M.2 Data Generation

Results are aggregated through:
- `Chapter4ResultsAnalyzer.hpp`: Aggregates simulation batches
- `ResultsComparator.hpp`: Comparative analysis across configurations
- `ParametricOptimizationPlotter.hpp`: Generates figures for chapters

---

## N. Development Workflow

### N.1 Adding New Analysis

1. Create new file in `Modelling/Analyzer/` (e.g., `NewAnalysis.hpp`)
2. Inherit or implement analysis interface
3. Call from `RunAnalysis.hpp` orchestrator
4. Add build configuration if needed
5. Link into appropriate application

### N.2 Integrating New Data Source

1. Implement `ISensor` (or specific interface)
2. Add to `Data-Service/Sensor/`
3. Update `APIManager.hpp` to route requests
4. Test with `api-test/` application

### N.3 Database Schema Changes

1. Modify `database-common.hpp` data structures
2. Update `SolarDatabaseManager.cpp` schema creation
3. Provide migration logic for existing databases
4. Document schema changes in comments

---

## O. Quick Reference: Key Files to Modify

| Task | Primary Files |
|------|---------------|
| Add collector type | `Collector/Collector.hpp`, create subclass |
| Add tracking mode | `Collector/Collector.hpp` (TrackingArchitecture enum), `Modelling/Analyzer/TrackingArchitecture.hpp` |
| Add material | `Configs/material.conf` or modify `Include/MaterialsProperties.hpp` |
| Add analysis | `Modelling/Analyzer/NewAnalysis.hpp` + `RunAnalysis.hpp` |
| Add sensor | `Data-Service/Sensor/NewSensor.h`, update `APIManager.hpp` |
| Change database schema | `Database-Service/database-common.hpp`, `SolarDatabaseManager.cpp` |
| Adjust simulation params | `Configs/sim.conf`, `Configs/run.conf` |

---

## P. Conclusion

The Parabolic Solar Tracker codebase exemplifies modern C++ software engineering principles applied to scientific research. Its modular architecture, clear separation of concerns, and extensibility make it suitable for ongoing research, development, and deployment in the solar energy domain. The integration of ray tracing, thermal modeling, and tracking algorithms provides a comprehensive platform for exploring advanced solar collector technologies.

This appendix serves as a reference for understanding the codebase structure, making it accessible to researchers, developers, and dissertation readers seeking to understand or build upon this work.

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Related Documentation:** [README.md](README.md), [TRACKING_IMPLEMENTATION.md](TRACKING_IMPLEMENTATION.md)
