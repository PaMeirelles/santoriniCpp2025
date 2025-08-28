#include "gtest/gtest.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include "../test_helpers.h"
#include <vector>
#include <optional>
#include <algorithm>

namespace Santorini {

TEST(HermesTests, can_survive_by_standing_still) {
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[2] = 2;
    blocks[5] = 2;
    blocks[6] = 2;
    blocks[7] = 2;
    Board board = create_board(blocks, {0, 1}, {3, 4}, 1, Constants::God::HERMES, Constants::God::ARTEMIS);
    EXPECT_EQ(board.check_state(), 0) << "Hermes should not lose if they can build without moving";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HermesTests, cannot_move_up_while_using_power) {
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    // Square 1 is height=1, so moving from 0 to 1 is an "up" move
    blocks[1] = 1;
    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::HERMES, Constants::God::ARTEMIS);

    // The move from 0->1 is an upward move, which is invalid for Hermes's chain movement.
    Moves::HermesMove move(0, {1, 2}, 3);
    EXPECT_FALSE(board.move_is_valid(move)) << "Hermes's chain move should not allow upward movement";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HermesTests, can_move_on_h1) {
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 1);
    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::HERMES, Constants::God::ARTEMIS);

    // This is a horizontal chain move on a flat plane of height 1. This should be a valid move.
    Moves::HermesMove move(0, {1, 2}, 3);
    EXPECT_TRUE(board.move_is_valid(move)) << "Hermes should be able to move horizontally on any flat level";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HermesTests, not_dead_on_h1) {
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 1);
    blocks[2] = 3;
    blocks[5] = 3;
    blocks[6] = 3;
    blocks[7] = 3;

    Board board = create_board(blocks, {0, 1}, {3, 4}, 1, Constants::God::HERMES, Constants::God::ARTEMIS);

    int state = board.check_state();
    EXPECT_EQ(state, 0) << "Hermes can survive by moving to a different horizontal spot and building";
}

} // namespace Santorini