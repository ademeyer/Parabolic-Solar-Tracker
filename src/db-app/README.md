# Database Application

This application manages simulation data storage and retrieval for the Solar Collector Simulation library. It interfaces with a database to store results, materials, and configurations for historical analysis and reuse.

It supports data persistence for long-term studies.

---

## Features

- **Data Storage:**  
  Saves simulation results, materials, and collector specs.  

- **Querying:**  
  Retrieves data for analysis or visualization.  

- **Integration:**  
  Connects with other apps for data sharing.  

---

## Usage

### Running the Application

1. Build the project (see root README.md).  
2. Set up a database (e.g., SQLite).  
3. Navigate to the application directory:  
   ```sh
   cd /src/db-app
   ```  
4. Run the executable:  
   ```sh
   ./db_app
   ```  

### Example Output

Database operations, e.g.:  
```
Stored: Collector PDC-1 results
Retrieved: 5 simulation records
...
```

### Customizing Database

Edit `db_app.cpp` to configure database connections, schemas, or queries.

---

## Dependencies

- Core library (`Collector.hpp`)  
- Database library (e.g., SQLite)  

---

## Building

This application is built as part of the main project:  
```sh
cd /build
make db_app  # Or build all with 'make'
```

Ensure `db_app.cpp` is included in your CMakeLists.txt.

---

## File Structure

- `db_app.cpp`: Main code for database operations.  

---

For more details, refer to the root README.md or code comments.