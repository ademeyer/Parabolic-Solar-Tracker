# Plate Test Application

This application is a test suite for the Solar Collector Simulation library, specifically demonstrating the thermal and optical performance of Parabolic Dish and Flat Plate collectors. It runs simulations using predefined collector specifications, weather data, and solar radiation inputs, then outputs thermal properties and generates 3D ray path visualizations.

It serves as an example of how to use the core `Collector` classes and integrates with ray tracing for flux mapping.

---

## Features

- **Collector Testing:**  
  Tests multiple Parabolic Dish and Flat Plate collectors with varying materials and geometries.  

- **Thermal Analysis:**  
  Computes intercepted energy, heat losses (conduction/convection), optical efficiency, and temperatures.  

- **Ray Tracing Visualization:**  
  Uses `RayTraceVisualizer` to plot 3D ray paths for analysis.  

- **Configurable Inputs:**  
  Supports custom weather, solar, date/time, and location data.  

---

## Usage

### Running the Application

1. Build the project (see root README.md).  
2. Navigate to the application directory:  
    ```sh
    cd /Parabolic-Solar-Tracker/src/plate-test
    ```  
3. Build project:  
    ```sh
    mkdir build && cd build
    cmake .. -DBUILD_PLATE_TEST
    ``` 
4. Run the executable:  
    ```sh
    ./plate_test
    ```  

### Example Output

The application prints thermal properties for each collector and generates visualization files (e.g., "single_loc" plots).

Sample output:  
```
=========== PDC-1 thermal properties ===========
SunXmin: ... [unit]
Total Intercepted Ray: ... [unit]
...
```

### Customizing Tests

Edit `plate_test.cpp` to:  
- Add/remove collectors in the `specs` vector.  
- Modify weather/solar data.  
- Change visualization parameters.  

---

## Dependencies

- Core library (`Collector.hpp`, `MaterialsProperties.hpp`)  
- Ray tracing (`SolTraceModel.h`, `RayTraceVisualizer.hpp`)  
- External libraries: SolTrace, SPA  

---

## Building

This application is built as part of the main project:  
```sh
cd /home/laplace/phd-project/build
make plate_test  # Or build all with 'make'
```

Ensure `plate_test.cpp` is included in your CMakeLists.txt.

---

## File Structure

- `plate_test.cpp`: Main application code with test logic and output functions.  

---

For more details, refer to the root README.md or code comments.