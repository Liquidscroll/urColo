cmake_minimum_required(VERSION 3.20...3.37)
project(urColo LANGUAGES CXX)

option(URCOLO_IMGUI_BACKEND "OpenGL (GL) or DirectX 12 (DX12) backend" "DX12")
string(TOUPPER "${URCOLO_IMGUI_BACKEND}" URCOLO_IMGUI_BACKEND)

option(SANITIZE_EXE "Set to apply address sanitization to the output exe." OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    add_compile_definitions(NOMINMAX) 
endif()

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

if(NOT WIN32)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
endif()


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
)
list(APPEND IMGUI_INCLUDE_DIRS
        ${IMGUI_DIR}
        ${IMGUI_DIR}/misc/cpp
        ${IMGUI_DIR}/backends)

if(WIN32)
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp)

    if(URCOLO_IMGUI_BACKEND STREQUAL "DX12")
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_imp_dx12.cpp)
        set(IMGUI_PLATFORM_LIBS d3d12 dxgi dxguid d3dcompiler_47)
    else()
        list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
        set(IMGUI_PLATFORM_LIBS opengl32)
    endif()
else()
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    set(IMGUI_PLATFORM_LIBS glfw OpenGL::GL)
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_INCLUDE_DIRS})
target_link_libraries(imgui PUBLIC ${IMGUI_PLATFORM_LIBS})

if (MSVC)
    target_compile_options(imgui PRIVATE /W4 /wd4189 /wd4100 /wd4180 /wd4505)
else()
    target_compile_options(imgui PRIVATE
        -Wno-unused-parameter
        -Wno-missing-field-initializers)
endif()

add_library(urColo_lib STATIC
    urColo/Colour.cpp
    urColo/PaletteGenerator.cpp
    urColo/ImageUtils.cpp
    urColo/Model.cpp
    urColo/Gui.cpp
    urColo/Gui/WindowManager.cpp
    urColo/Gui/Tab.cpp
    urColo/Gui/HighlightsTab.cpp
    urColo/Gui/ContrastTestTab.cpp
    urColo/Gui/GenSettingsTab.cpp
    urColo/Gui/PaletteGenTab.cpp
    external/stb_image.cpp
)
set_source_files_properties(external/stb_image.cpp PROPERTIES COMPILE_FLAGS "-w")

target_include_directories(urColo_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/urColo
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

if (NOT MSVC)
    target_compile_options(urColo_lib PRIVATE
        ${GCC_CLANG_WARNING_FLAGS}
        ${SANITIZER_FLAGS})
endif()

add_executable(urColo
    urColo/main.cpp
)

target_include_directories(urColo PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/urColo
   ${IMGUI_DIR}
   ${IMGUI_DIR}/backends)

if (NOT MSVC)
    target_compile_options(urColo PRIVATE
        ${GCC_CLANG_WARNING_FLAGS}
        ${SANITIZER_FLAGS})
endif()
if(SANITIZE_EXE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(urColo PRIVATE -fsanitize=address,undefined)
endif()

target_link_libraries(urColo_lib PRIVATE imgui ${IMGUI_PLATFORM_LIBS} nlohmann_json::nlohmann_json)

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
