cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(merge_plot)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(CMAKE_CXX_STANDARD 20)
find_package(ROOT REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
include("${ROOT_USE_FILE}")
include_directories(
    ${ROOT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(merge_plot main.cxx)
target_link_libraries(merge_plot nlohmann_json::nlohmann_json ROOT::Tree ROOT::Hist ROOT::Gpad)
install(TARGETS merge_plot DESTINATION bin)