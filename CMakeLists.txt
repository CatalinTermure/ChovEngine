cmake_minimum_required(VERSION 3.25)
project(LabsExtra)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_DIR "SDL2-2.26.5/cmake") # Set this to the directory containing SDL2Config.cmake
find_package(SDL2 REQUIRED)

add_executable(LabsExtra src/rendering/main.cpp)
find_package(Vulkan)
include_directories("${Vulkan_INCLUDE_DIRS}")
target_link_libraries(LabsExtra ${Vulkan_LIBRARIES})
target_link_libraries(LabsExtra ${SDL2_LIBRARIES})

