#include "gtest/gtest.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include "../test_helpers.h" // Assuming test_helpers is in the parent directory of current test file
#include <vector>
#include <optional>
#include <algorithm>

namespace Santorini {


TEST(HephaestusTests, cannot_build_twice_on_different_squares) {
    /**
     * Hephaestus's second build must be on the same square as the first.
     */
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::HEPHAESTUS, Constants::God::ARTEMIS);

    // Attempt from 0->1, build1=2, build2=3 => invalid because build_sq_1 != build_sq_2
    // HephaestusMove constructor:sqfrom,sqto,sqb1, optional<sq> b2 = nullopt
    Moves::HephaestusMove move(0, 1, 2, 3);
    EXPECT_FALSE(board.move_is_valid(move)) << "Hephaestus should not be able to build twice on different squares";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HephaestusTests, can_build_only_once_if_desired) {
    /**
     * Hephaestus's second build is optional.
     */
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::HEPHAESTUS, Constants::God::ARTEMIS);

    // From 0->1, build at 2 only once. No second build square.
    // HephaestusMove constructor:sqfrom,sqto,sqb1, optional<sq> b2 = nullopt
    Moves::HephaestusMove move(0, 1, 2); // Only build_sq_1 is provided
    EXPECT_TRUE(board.move_is_valid(move)) << "Hephaestus should be able to build only once if desired";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HephaestusTests, cannot_dome_on_second_build) {
    /**
     * If the second build would raise the block from 3->4, that's a dome.
     * Hephaestus's code forbids building a dome on the second block
     * (the code checks `self.blocks[build_sq_2] <= 2`).
     */
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[2] = 2; // Square 2 is height 2

    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::HEPHAESTUS, Constants::God::ARTEMIS);

    // Gray moves 0->1, builds twice on square 2.
    // Initial blocks[2] = 2.
    // First build: blocks[2] becomes 3.
    // Second build: blocks[2] becomes 4 (dome). This should be invalid for Hephaestus's second build.
    Moves::HephaestusMove move(0, 1, 2, 2);
    EXPECT_FALSE(board.move_is_valid(move)) << "Hephaestus should not be able to dome on the second build";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HephaestusTests, can_dome_normally) {
    /**
     * Hephaestus can build a single block that results in a dome
     * if the square was already 3. That's a normal single build to 4, allowed.
     */
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[2] = 3; // Square 2 is height 3, so a single build will dome it.

    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::HEPHAESTUS, Constants::God::ARTEMIS);

    // Single build from 0->1, build at 2.
    // blocks[2] goes from 3 to 4 (dome). This is a single build and should be valid.
    Moves::HephaestusMove move(0, 1, 2);
    EXPECT_TRUE(board.move_is_valid(move)) << "Hephaestus should be able to dome with a single build";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HephaestusTests, misc) {
    // This test checks a specific board state and expects a certain outcome from check_state().
    // The exact state value (-1 in Python) corresponds to a loss for the current player
    // (which is the player whose turn it is when check_state() is called).
    Board board("0N3N3N0G2G2N3N4N4N4N2N3N3N1B3N0N1N3N0B1N0N0N1N0N1N0580");
    int state = board.check_state();
    EXPECT_EQ(state, -1) << "Board state should be -1 for this specific scenario (loss for current player)";
}

} // namespace Santorini