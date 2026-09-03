cmake_minimum_required(VERSION 3.20)
project(OperationAutocraft CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add JSON library
add_subdirectory(third_party/json EXCLUDE_FROM_ALL)

# Add ImGui files manually since it doesn't have a modern CMakeLists.txt out of the box
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Application executable
file(GLOB APP_SOURCES src/*.cpp)

add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Operation Autocraft")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    nlohmann_json::nlohmann_json
    d3d11
    d3dcompiler
    dxgi
    dwmapi
)

# Compiler flags for MSVC
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /MP)
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'")
endif()
