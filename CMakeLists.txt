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

add_executable(merge_stack merge_stack.cxx)
target_link_libraries(merge_stack nlohmann_json::nlohmann_json ROOT::Tree ROOT::Hist ROOT::Gpad)
install(TARGETS merge_stack DESTINATION bin)

add_executable(draw2d draw2d.cxx)
target_link_libraries(draw2d nlohmann_json::nlohmann_json ROOT::Tree ROOT::Hist ROOT::Gpad)
install(TARGETS draw2d DESTINATION bin)

add_executable(drawall2d drawall2d.cxx)
target_link_libraries(drawall2d nlohmann_json::nlohmann_json ROOT::Tree ROOT::Hist ROOT::Gpad)
install(TARGETS drawall2d DESTINATION bin)

add_executable(2d_renormalize 2d_renormalize.cxx)
target_link_libraries(2d_renormalize nlohmann_json::nlohmann_json ROOT::Tree ROOT::Hist ROOT::Gpad)
install(TARGETS 2d_renormalize DESTINATION bin)

add_executable(2dproj 2dproj.cxx)
target_link_libraries(2dproj nlohmann_json::nlohmann_json ROOT::Tree ROOT::Hist ROOT::Gpad ROOT::ASImage)
install(TARGETS 2dproj DESTINATION bin)