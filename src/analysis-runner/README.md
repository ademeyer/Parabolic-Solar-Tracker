# Analysis Runner Application

This application runs comprehensive analysis on solar collector simulations, processing multiple scenarios and generating reports on thermal performance, efficiency, and losses. It integrates with the core library to evaluate collectors under varying conditions.

It is designed for batch processing and performance benchmarking.

---

## Features

- **Batch Analysis:**  
  Runs simulations for multiple collectors and conditions.  

- **Report Generation:**  
  Outputs detailed reports on energy interception, losses, and temperatures.  

- **Optimization Support:**  
  Helps identify optimal materials and geometries.  

---

## Usage

### Running the Application

1. Build the project (see root README.md).  
2. Navigate to the application directory:  
   ```sh
   cd /Parabolic-Solar-Tracker/src/analysis-runner
   mkdir build && cd build
   cmake .. -DBUILD_RUNNER_APP
   ```  
3. Run the executable:  
   ```sh
   ./analysis_runner
   ```  

### Example Output

Generates reports like:  
```
Collector: PDC-1
Intercepted Energy: 1500 W
Efficiency: 0.85
...
```

### Customizing Runs

Edit `analysis_runner.cpp` to define scenarios, collectors, and output formats.

---

## Dependencies

- Core library (`Collector.hpp`, `MaterialsProperties.hpp`)  
- Ray tracing (`SolTraceModel.h`)  

---

## Building

This application is built as part of the main project:  
```sh
cd /home/laplace/phd-project/build
make analysis_runner  # Or build all with 'make'
```

Ensure `analysis_runner.cpp` is included in your CMakeLists.txt.

---

## File Structure

- `analysis_runner.cpp`: Main code for running analyses and generating reports.  

---

For more details, refer to the root README.md or code comments.