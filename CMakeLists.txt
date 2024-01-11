cmake_minimum_required(VERSION 3.25)
project(LabsExtra)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_DIR "SDL2-2.26.5/cmake") # Set this to the directory containing SDL2Config.cmake
find_package(SDL2 REQUIRED)

find_package(glm REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)

add_library(ProjectObjects src/objects/scene.cpp)
target_include_directories(ProjectObjects PUBLIC include)
target_link_libraries(ProjectObjects glm::glm absl::base absl::hash absl::log absl::status)

add_library(ProjectRendering src/rendering/mesh.cpp
        src/rendering/window.cpp
        src/rendering/camera.cpp
        src/rendering/libraries_initializer.cpp)
target_include_directories(ProjectRendering PUBLIC include)
target_link_libraries(ProjectRendering glm::glm absl::base absl::hash absl::log absl::status)

add_library(ProjectRenderingOpenGL
        src/rendering/opengl/renderer.cpp
        src/rendering/opengl/shader.cpp
        src/rendering/opengl/pipeline.cpp
        src/rendering/opengl/texture.cpp
        src/rendering/opengl/texture_allocator.cpp
        src/rendering/opengl/render_object.cpp
        src/rendering/opengl/shader_allocator.cpp
        src/rendering/opengl/uniform.cpp)
target_link_libraries(ProjectRenderingOpenGL GLEW::GLEW absl::base absl::log)
target_include_directories(ProjectRenderingOpenGL PUBLIC include)

add_library(ProjectApplication src/application.cpp)
target_include_directories(ProjectApplication PUBLIC include)
target_link_libraries(ProjectApplication ProjectObjects ProjectRendering absl::base absl::hash absl::log absl::status)

add_executable(LabsExtra src/main.cpp
        src/game.cpp
        src/demo_game.cpp
)
target_include_directories(LabsExtra PUBLIC include)

set(GLSLC_EXECUTABLE "glslc.exe")
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vulkan)

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

find_package(Vulkan)
include_directories("${Vulkan_INCLUDE_DIRS}")
target_link_libraries(LabsExtra ${SDL2_LIBRARIES} glm::glm absl::base absl::log absl::status)
target_link_libraries(LabsExtra ProjectApplication ProjectRenderingOpenGL)
target_include_directories(LabsExtra PUBLIC include)
