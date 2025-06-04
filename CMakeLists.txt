cmake_minimum_required(VERSION 3.37)
project(urColo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GCC_CLANG_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wformat=2
    -Wformat-security
    -Wnull-dereference
    -Wstack-protector
    -Wundef
    -Wcast-align
    -Werror
)

set(SANITIZER_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
    AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZER_FLAGS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fstack-protector-strong
    )
endif()

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# imgui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
set(IMGUI_INCLUDE_DIRS
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

add_library(urColo_lib STATIC
    urColo/Colour.cpp
    urColo/Contrast.cpp
    urColo/PaletteGen.cpp
    urColo/Model.cpp
    urColo/Gui.cpp
)

target_include_directories(urColo_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/urColo
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${IMGUI_INCLUDE_DIRS}
)

add_executable(urColo
    urColo/main.cpp
    ${IMGUI_SOURCES}
)

if (WIN32)
    target_link_libraries(urColo PRIVATE urColo_lib glfw opengl32.lib)
else()
    target_link_libraries(urColo PRIVATE urColo_lib glfw OpenGL::GL)
endif()

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "GLFW version: ${glfw3_VERSION_STRING}")
message(STATUS "OpenGL version: ${OpenGL_VERSION_STRING}")
