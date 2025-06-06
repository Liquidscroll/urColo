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
    test_serialization.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../urColo/Colour.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../urColo/PaletteGenerator.cpp
)

target_include_directories(urColo_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../external
)

target_link_libraries(urColo_tests PRIVATE doctest::doctest nlohmann_json::nlohmann_json)

target_compile_options(urColo_tests PRIVATE ${GCC_CLANG_WARNING_FLAGS} ${SANITIZER_FLAGS})

add_test(NAME urColo_tests COMMAND urColo_tests)
