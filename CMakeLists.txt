cmake_minimum_required(VERSION 3.20)
project(Paladini_7.0_Lunar)

set(CMAKE_CXX_STANDARD 20)

# ############################################################################
# # ENGINE LIBRARY
# # Create a library that contains all the core engine logic.
# # Both the main executable and the tests will use this.
# ############################################################################
add_library(
        santorini_engine
        src/board.cpp
        src/moves.cpp
)

# ############################################################################
# # MAIN EXECUTABLE
# # This is your main application.
# ############################################################################
add_executable(Paladini_7.0_Lunar src/main.cpp src/board.cpp src/moves.cpp)

# Link the main executable against the engine library.
target_link_libraries(
        Paladini_7.0_Lunar
        PRIVATE santorini_engine
)

# ############################################################################
# # TESTING
# ############################################################################
enable_testing()

# Add the googletest subdirectory (assuming it's in extern/googletest)
add_subdirectory(extern/googletest)

# Create an executable for your tests.
add_executable(
        run_tests
        tests/test_parsing.cpp
        tests/test_general_rules.cpp
        tests/test_hashing.cpp
        tests/test_validation.cpp
        tests/godSpecific/test_apollo.cpp
        tests/godSpecific/test_artemis.cpp
        tests/godSpecific/test_athena.cpp
        tests/godSpecific/test_atlas.cpp
        tests/godSpecific/test_demeter.cpp
        tests/godSpecific/test_hephaestus.cpp
        tests/godSpecific/test_hermes.cpp
        tests/godSpecific/test_minotaur.cpp
        tests/godSpecific/test_pan.cpp
        tests/godSpecific/test_prometheus.cpp
        tests/test_unmake_move_tests.cpp
        tests/test_transposition_table.cpp
        tests/test_evaluation.cpp
        tests/test_climber_quiet.cpp
)

# Link the test executable against both gtest and your engine library.
target_link_libraries(
        run_tests
        PRIVATE
        gtest_main
        santorini_engine
)

# Discover and add the tests to the CTest runner.
include(GoogleTest)
gtest_discover_tests(run_tests)
