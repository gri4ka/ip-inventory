# IP Inventory REST API

C++11 REST API for managing IP address inventory.

## Structure

```
ip-inventory/
  CMakeLists.txt
  sql/schema.sql
  third_party/
    httplib.h -- C++11 header-only HTTP library
    json.hpp -- C++11 header-only JSON library
  headers/
    cleanup.hpp
    handlers.hpp
    ip_util.hpp
    logger.hpp
    odbc_common.hpp
    odbc_db.hpp
  src/
    cleanup.cpp
    handlers.cpp
    ip_util.cpp
    main.cpp
    odbc_common.cpp
    odbc_db.cpp
    
```

## Description
### Architecture
The application is structured into several layers:
- **HTTP Layer**: This layer handles incoming HTTP requests and routes them to the appropriate handlers. It is made of the `main.cpp` and `handlers.cpp` It uses the `httplib.h` library to create a REST API server.
- **Database Layer**: Handles the business logic related to db operations. It is implemented in `odbc_db.cpp` and uses `odbc_common.cpp` to interact with a PostgreSQL database.
- **Infrastructure Layer**: This layer includes infrastructure functionalities such as wrappers for ODBC handles(`odbc_common.cpp`), logging (`logger.hpp`), IP address manipulation (`ip_util.hpp`), and cleanup tasks (`cleanup.hpp`).

### Project Decisions
- Windows only: Recuded overall complexity for myself in the setup of the database and db connection. Otherwise the code is mostly platform agnostic, except for the sockets.  
- http library: `httplib.h` was chosen for its compatibility, simplicity and header-only nature, which allows for easy integration.
- json library: `json.hpp` was chosen for its ease of use and header-only nature.
- RAII for ODBC handles: To ensure proper resource management and prevent leaks, RAII wrappers were implemented for ODBC handles.
- Separate threads for API server and cleanup: The API server runs in the main thread, while a separate thread is dedicated to periodically cleaning up expired reservations. This allows handling of API requests without blocking due to cleanup operations.
- Full GUI for all API operations: This made my life way easier when testing. The requirement was to only have a GUI for the ip-pool method, however I decided to extend it for all. 
## Setup

### 1. Download dependencies

Dependency libaries are included in the `third_party` directory. You can find more information about them them from the following links:
- [httplib.h](https://github.com/yhirose/cpp-httplib/)
- [json.hpp](https://github.com/nlohmann/json/)

Briefly, `httplib.h` is a C++11 header-only HTTP library used for creating the REST API server, while `json.hpp` is a C++11 header-only JSON library used for parsing and generating JSON data in the API.
### 2. Start PostgreSQL

```bash
psql "postgresql://postgres:postgres@localhost:5433/postgres" -c "CREATE DATABASE ipinv;"
psql "postgresql://postgres:postgres@localhost:5433/ipinv" -f sql/schema.sql
```

### 3. Build

If using Visual Studio, open the repository folder and build the project. For command line:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 4. Run
After building if using Visual Studio, run the project. You can also find the executable in the `ip_inventory\out\build\x64-debug\` folder. For command line(assuming you didn't move from the `build` folder):
```bash
..\out\build\x64-debug\ip_inventory.exe
```

### 5. Testing
Ideally you would open the `frontend.html` and use it to manually test the API. However, you can also use `curl` or any API testing tool like Postman to send requests to the API endpoints.
I personally failed to test with curl so I can not comment on that.
An example Postman request could be: `Type: POST, URL: http://localhost:8080/ip-inventory/ip-pool, Body: {"ipAddresses": [{"ip": "9.9.9.9", "ipType": "IPv4"}]}`.
You can also view the swagger yalm. I am not familiar with the technology and used editor.swagger.io to make it. I hope it works fine on whatever you use to inspect it.

Configuration via environment variables(found in `CMakePresets.json`):

- `IPINV_ODBC` — ODBC connection string
- `IPINV_PORT` — HTTP port (default 8080)
- `IPINV_RESERVE_TTL_SEC` — reservation TTL in seconds (default 60)
- `IPINV_CLEANUP_INTERVAL_SEC` — cleanup interval in seconds (default 10)

## API Endpoints

- `POST /ip-inventory/ip-pool` — add IPs to the pool
- `POST /ip-inventory/reserve-ip` — reserve IP for a service
- `POST /ip-inventory/assign-ip-serviceId` — assign reserved IP
- `POST /ip-inventory/terminate-ip-serviceId` — release IP
- `POST /ip-inventory/serviceId-change` — transfer IPs between services
- `GET /ip-inventory/serviceId?serviceId=xxx` — query IPs by service
