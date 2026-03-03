cmake_minimum_required(VERSION 3.15)
project(ip_inventory CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg provides this config package name
#find_package(libpqxx CONFIG REQUIRED)

add_executable(ip_inventory
  "src/odbc_common.cpp"
  "src/main.cpp"
  #src/db.cpp
  #src/ip_util.cpp
  #src/cleanup_job.cpp
)

target_link_libraries(ip_inventory PRIVATE odbc32 ws2_32)