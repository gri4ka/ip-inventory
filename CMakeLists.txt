cmake_minimum_required(VERSION 3.15)
project(ip_inventory CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ip_inventory
  "src/odbc_common.cpp"
  "src/main.cpp"
  "src/odbc_db.cpp"
  "src/ip_util.cpp"
  "src/handlers.cpp"
  #src/cleanup_job.cpp
)

target_link_libraries(ip_inventory PRIVATE odbc32 ws2_32)