cmake_minimum_required(VERSION 3.20...3.37)
project(urColo LANGUAGES CXX)

option(SANITIZE_EXE "Set to apply address sanitization to the output exe." OFF)

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
    AND SANITIZE_EXE
    AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZER_FLAGS
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fstack-protector-strong
    )
endif()

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)


# nlohmann/json
include(FetchContent)
FetchContent_Declare(json
                     URL
                     https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# imgui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
set(IMGUI_INCLUDE_DIRS
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIRS})
target_compile_options(imgui PRIVATE
    -Wno-unused-parameter
    -Wno-missing-field-initializers)

add_library(urColo_lib STATIC
    urColo/Colour.cpp
    urColo/Contrast.cpp
    urColo/PaletteGenerator.cpp
    urColo/ImageUtils.cpp
    urColo/Model.cpp
    urColo/Gui.cpp
    urColo/Gui/Tab.cpp
    urColo/Gui/HighlightsTab.cpp
    urColo/Gui/ContrastTestTab.cpp
    external/stb_image.cpp
)
set_source_files_properties(external/stb_image.cpp PROPERTIES COMPILE_FLAGS "-w")

target_include_directories(urColo_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/urColo
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_compile_options(urColo_lib PRIVATE
    ${GCC_CLANG_WARNING_FLAGS}
    ${SANITIZER_FLAGS})

add_executable(urColo
    urColo/main.cpp
)

target_include_directories(urColo PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/urColo)

target_compile_options(urColo PRIVATE
    ${GCC_CLANG_WARNING_FLAGS}
    ${SANITIZER_FLAGS})
if(SANITIZE_EXE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(urColo PRIVATE -fsanitize=address,undefined)
endif()

if (WIN32)
    target_link_libraries(urColo_lib PRIVATE imgui glfw opengl32.lib nlohmann_json::nlohmann_json)
else()
    target_link_libraries(urColo_lib PRIVATE imgui glfw OpenGL::GL nlohmann_json::nlohmann_json)
endif()

target_link_libraries(urColo PRIVATE urColo_lib nlohmann_json::nlohmann_json)

find_program(CPPCHECK_EXECUTABLE cppcheck)
if(CPPCHECK_EXECUTABLE)
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_EXECUTABLE}
                --enable=all
                --inline-suppr
                --project=${CMAKE_BINARY_DIR}/compile_commands.json
                --suppress=*:*external/*
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck static analysis..."
        VERBATIM
    )

    message(STATUS "cppcheck target added.
        Run with 'cmake -- build . --target cppcheck' or 'make cppcheck'.")
else()
    message(WARNING "cppcheck not found.
        The cppcheck target will not be available.")
endif()

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "GLFW version: ${glfw3_VERSION_STRING}")
message(STATUS "OpenGL version: ${OpenGL_VERSION_STRING}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Using GCC/Clang warning flags.")
    if(SANITIZE_EXE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Sanitizers enabled for Debug build (GCC/Clang).")
    endif()
elseif(MSVC)
    message(STATUS "Using MSVC warning flags.")
endif()
if(TARGET cppcheck)
    message(STATUS "cppcheck enabled. Invoke with 'make cppcheck'.")
endif()

enable_testing()
add_subdirectory(tests)
