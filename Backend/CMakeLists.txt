cmake_minimum_required(VERSION 3.14)
project(LibraryBackend)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# Fetch cpp-httplib (Zero dependencies, works perfectly on Windows)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# Fetch standard JSON library
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

add_executable(server main.cpp)

# Link libraries. ws2_32 is required by Windows for network sockets.
target_link_libraries(server PRIVATE httplib::httplib nlohmann_json::nlohmann_json ws2_32)