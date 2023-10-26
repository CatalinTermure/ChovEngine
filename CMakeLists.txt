cmake_minimum_required(VERSION 3.25)
project(LabsExtra)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_DIR "SDL2-2.26.5/cmake") # Set this to the directory containing SDL2Config.cmake
find_package(SDL2 REQUIRED)

set(glm_DIR glm/cmake/glm)
find_package(glm REQUIRED)

add_library(ProjectUtils src/utils/logging.cpp)
target_include_directories(ProjectUtils PUBLIC include/utils)

add_library(ProjectRendering src/rendering/camera.cpp)
target_include_directories(ProjectRendering PUBLIC include/rendering)
target_link_libraries(ProjectRendering glm::glm)

add_executable(LabsExtra src/main.cpp)
target_include_directories(LabsExtra PUBLIC include)

set(GLSLC_EXECUTABLE "glslc.exe")
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

file(GLOB SHADERS
        ${SHADER_SOURCE_DIR}/*.vert
        ${SHADER_SOURCE_DIR}/*.frag)

foreach (source IN LISTS SHADERS)
    get_filename_component(FILENAME ${source} NAME)
    add_custom_command(
            COMMAND ${GLSLC_EXECUTABLE} -o ${SHADER_BINARY_DIR}/${FILENAME}.spv ${source}
            OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
            DEPENDS ${source} ${SHADER_BINARY_DIR}
    )
    list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach ()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

add_dependencies(LabsExtra shaders)

find_package(VulkanMemoryAllocator CONFIG REQUIRED)
target_link_libraries(LabsExtra GPUOpen::VulkanMemoryAllocator)

find_package(Vulkan)
include_directories("${Vulkan_INCLUDE_DIRS}")
target_link_libraries(LabsExtra ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES} glm::glm)
target_link_libraries(LabsExtra ProjectUtils ProjectRendering)
