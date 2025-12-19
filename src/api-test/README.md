# API Test Application

This application tests the API interfaces of the Solar Collector Simulation library, including data input/output, collector initialization, and simulation calls. It validates functionality and error handling.

It is essential for ensuring API reliability in integrated systems.

---

## Features

- **API Validation:**  
  Tests collector methods, data structures, and error responses.  

- **Automated Testing:**  
  Runs unit-like tests for library components.  

- **Debugging Support:**  
  Logs failures and edge cases.  

---

## Usage

### Running the Application

1. Build the project (see root README.md).  
2. Navigate to the application directory:  
   ```sh
   cd /Parabolic-Solar-Tracker/src/api-test
   mkdir build && cd build
   cmake .. -DBUILD_API_APP
   ```  
3. Run the executable:  
   ```sh
   ./api_test
   ```  

### Example Output

Test results, e.g.:  
```
Test 1: Collector Initialization - PASSED
Test 2: Thermal Calculation - FAILED (invalid input)
...
```

### Customizing Tests

Edit `api_test.cpp` to add new test cases or modify existing ones.

---

## Dependencies

- Core library (`Collector.hpp`)  
- Testing frameworks (if used, e.g., Google Test)  

---

## Building

This application is built as part of the main project:  
```sh
cd /build
make api_test  # Or build all with 'make'
```

Ensure `api_test.cpp` is included in your CMakeLists.txt.

---

## File Structure

- `api_test.cpp`: Main code for API testing logic.  

---

For more details, refer to the root README.md or code comments.