#include "gtest/gtest.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include "../test_helpers.h"
#include <vector>
#include <optional>
#include <algorithm>

namespace Santorini {


TEST(PrometheusTests, can_move_normally_if_you_want) {
    /**
     * If Prometheus does not do the optional build first,
     * then it's effectively a normal single-step move + build.
     */
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::PROMETHEUS, Constants::God::ARTEMIS);

    // From 0->1, then build at 2, with no optional build => valid
    Moves::Move move(0, 1, 2, Constants::God::PROMETHEUS);
    EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Prometheus should be able to make a normal move without a pre-build";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(PrometheusTests, second_build_must_be_before_moving) {
    /**
     * If 'optional_build' is not None, it is done before the move.
     * Then we do a normal build after the move. That's the sequence.
     */
    // We'll just verify that the code requires from_sq to be adjacent to optional_build
    // (which it does: `_build_ok(from_sq, from_sq, optional_build_sq)`).
    // Then we do from_sq->to_sq, build_sq => final building.
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::PROMETHEUS, Constants::God::ARTEMIS);

    // Suppose from=0, optional_build=5 => adjacent to 0.
    // Then move to 1 => valid single step from new worker pos.
    // Then build at 2 => adjacent to 1.
    Moves::Move move(0, 1, 2, Constants::God::PROMETHEUS);
    move.extra_build_sq = 5;
    EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Prometheus should be able to perform a pre-build, move, and final build";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(PrometheusTests, cannot_move_up_if_built_before_moving) {
    std::array<sq_i , 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[1] = 1; // Square 1 is height 1. Moving from 0 (height 0) to 1 is an "up" move.
    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::PROMETHEUS, Constants::God::ARTEMIS);

    // If we set optional_build=5 (adjacent to 0), that means we built first.
    // Prometheus cannot move up after an optional build.
    Moves::Move move(0, 1, 2, Constants::God::PROMETHEUS); // Pre-build at 5, then move from 0->1 (up)
    move.extra_build_sq = 5;
    EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Prometheus cannot move up if a pre-build was performed";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(PrometheusTests, cannot_move_up_to_newly_built) {
    std::array<sq_i , 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    Board board = create_board(blocks, {0, 1}, {23, 24}, 1, Constants::God::PROMETHEUS, Constants::God::ARTEMIS);

    // If we set optional_build=5, and then try to move to_sq=5.
    // This means Prometheus pre-builds on 5, making its height 1.
    // Then, the worker tries to move from 0 (height 0) to 5 (newly built height 1),
    // which is an upward move to a square it just built on. This should be invalid.
    // The `_is_valid_prometheus` check `_blocks[move.to_sq] + temp_h_adj > _blocks[move.from_sq]`
    // should catch this, where `temp_h_adj` accounts for the pre-build on `to_sq`.
    Moves::Move move(0, 5, 10, Constants::God::PROMETHEUS); // Pre-build at 5, then move from 0->5 (up to new build)
    move.extra_build_sq = 5;
    EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Prometheus cannot move up to a square just built on";
}
TEST(PrometheusTests, cannot_build_twice_on_height_3) {
    /**
     * Prometheus should not be able to perform a pre-build and a final build
     * on the same square if that square is already at height 3. The pre-build
     * would place a dome, making the final build invalid.
     */
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[5] = 3; // Target build square is already at height 3
    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::PROMETHEUS, Constants::God::ARTEMIS);

    // Attempt to:
    // 1. Pre-build on square 5 (height 3) from worker at 0. This places a dome.
    // 2. Move worker from 0 to 1.
    // 3. Final build on square 5 (now domed). This is invalid.
    Moves::Move move(0, 1, 5, Constants::God::PROMETHEUS);
    move.extra_build_sq = 5; // Pre-build on the same square.

    EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Prometheus cannot build twice on a square that starts at height 3";
}
} // namespace Santorini

