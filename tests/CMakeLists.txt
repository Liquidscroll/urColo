include(FetchContent)

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
)
FetchContent_MakeAvailable(doctest)

add_executable(urColo_tests
    test_contrast.cpp
    test_colour.cpp
    test_generator.cpp
    test_model.cpp
    test_serialization.cpp
    test_logger.cpp
    test_imageutils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../urColo/Colour.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../urColo/PaletteGenerator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../urColo/Model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../urColo/ImageUtils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../external/stb_image.cpp
)

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/../external/stb_image.cpp
    PROPERTIES COMPILE_FLAGS "-w")

target_include_directories(urColo_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../external
)

target_compile_definitions(urColo_tests PRIVATE
    TEST_ASSETS_DIR="${CMAKE_CURRENT_LIST_DIR}/assets")

target_link_libraries(urColo_tests PRIVATE doctest::doctest nlohmann_json::nlohmann_json)

if (NOT MSVC)
    target_compile_options(urColo_tests PRIVATE ${GCC_CLANG_WARNING_FLAGS} ${SANITIZER_FLAGS})
endif()

add_test(NAME urColo_tests COMMAND urColo_tests)
