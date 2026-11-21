cmake_minimum_required(VERSION 3.20)
project(Davi_3.0.0_Joker)

set(CMAKE_CXX_STANDARD 20)

# ############################################################################
# # DEPENDENCIES
# ############################################################################
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/libtorch/share/cmake/Caffe2")
find_package(Torch REQUIRED)

# ############################################################################
# # ENGINE LIBRARY
# ############################################################################
add_library(
        santorini_engine
        src/board.cpp
        src/moves.cpp
) #

# Link LibTorch to the engine.
# Using PUBLIC ensures that anything linking to santorini_engine
# (like your main app and tests) also gets the Torch headers/libs.
target_link_libraries(
        santorini_engine
        PUBLIC "${TORCH_LIBRARIES}"
)

# ############################################################################
# # MAIN EXECUTABLE
# ############################################################################
add_executable(Davi_3.0.0_Joker src/main.cpp src/board.cpp src/moves.cpp) # 

# MSVC + LibTorch CUDA needs warp_size forced into the link (see joker build.cmd)
if(MSVC)
    target_link_options(Davi_3.0.0_Joker PRIVATE "/INCLUDE:?warp_size@cuda@at@@YAHXZ")
endif()

target_link_libraries(
        Davi_3.0.0_Joker
        PRIVATE santorini_engine
) 

# ############################################################################
# # TESTING
# ############################################################################
enable_testing()

add_subdirectory(extern/googletest)

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
        tests/godSpecific/test_minotaur.cpp # 
        tests/godSpecific/test_pan.cpp
        tests/godSpecific/test_prometheus.cpp
        tests/test_unmake_move_tests.cpp
        tests/test_transposition_table.cpp
        tests/test_evaluation.cpp
        tests/test_climber_quiet.cpp
) 

target_link_libraries(
        run_tests
        PRIVATE
        gtest_main
        santorini_engine
) 

include(GoogleTest)

gtest_discover_tests(run_tests)

# Get the list of all DLLs in the libtorch/lib directory
file(GLOB TORCH_DLLS "${CMAKE_PREFIX_PATH}/lib/*.dll")

# Copy them to the directory where the executable is built
add_custom_command(TARGET Davi_3.0.0_Joker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${TORCH_DLLS}
        $<TARGET_FILE_DIR:Davi_3.0.0_Joker>
)

add_custom_command(TARGET Davi_3.0.0_Joker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/models/santorini_evaluator.pt"
        $<TARGET_FILE_DIR:Davi_3.0.0_Joker>
)