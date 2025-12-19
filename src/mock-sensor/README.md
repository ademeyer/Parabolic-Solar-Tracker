# Mock Sensor Application

This application simulates sensor data for testing the Solar Collector Simulation library. It generates mock environmental data (e.g., weather, solar radiation, location) to mimic real-world inputs without requiring physical sensors or external APIs.

It is useful for development, unit testing, and validating collector models under controlled conditions.

---

## Features

- **Data Simulation:**  
  Generates realistic weather, solar, and location data based on configurable parameters.  

- **Customizable Scenarios:**  
  Supports different time zones, latitudes, and weather conditions.  

- **Integration Testing:**  
  Feeds mock data into collector classes for thermal analysis.  

---

## Usage

### Running the Application

1. Build the project (see root README.md).  
2. Navigate to the application directory:  
   ```sh
   cd /Parabolic-Solar-Tracker/src/mock-sensor
   mkdir build && cd build
   cmake .. -DBUILD_SENSOR_APP
   ```  
3. Run the executable:  
   ```sh
   ./mock_sensor
   ```  

### Example Output

Outputs simulated data in JSON or console format, e.g.:  
```
Weather: Temp=25.0°C, Wind=5.0 m/s
Solar: DNI=1000.0 W/m², Elevation=45.0°
...
```

### Customizing Simulations

Edit `mock_sensor.cpp` to adjust data ranges, scenarios, or output formats.

---

## Dependencies

- Core library (`Collector.hpp`)  
- Standard C++ libraries  

---

## Building

This application is built as part of the main project:  
```sh
cd /home/laplace/phd-project/build
make mock_sensor  # Or build all with 'make'
```

Ensure `mock_sensor.cpp` is included in your CMakeLists.txt.

---

## File Structure

- `mock_sensor.cpp`: Main code for data generation and output.  

---

For more details, refer to the root README.md or code comments.