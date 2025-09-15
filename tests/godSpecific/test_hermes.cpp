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
    blocks[2] = 1;
    blocks[3] = 1;
    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::HERMES, Constants::God::ARTEMIS);

    // The move from 0->1 is an upward move, which is invalid for Hermes's chain movement.
    Moves::Move move(0, 3, 4, Constants::God::HERMES);
    EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Hermes's chain move should not allow upward movement";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(HermesTests, can_move_on_h1) {
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 1);
    Board board = create_board(blocks, {0, 10}, {23, 24}, 1, Constants::God::HERMES, Constants::God::ARTEMIS);

    // This is a horizontal chain move on a flat plane of height 1. This should be a valid move.
    Moves::Move move(0, 2, 3, Constants::God::HERMES);
    EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Hermes should be able to move horizontally on any flat level";
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

//----------------------------------------------------------------------------------------------------------------------

TEST(HermesTests, to_text_for_stand_still_move) {
    // In this position, a worker is at b2 (6). A valid move is to
    // stand still and build at the adjacent c2 (10).
    Board board("0N0N0N0N0N0N0G1N0N0N1N0B0G0B0N0N1N1N0N0N0N0N0N0N0N0600");
    using namespace Santorini::Moves;

    // Manually create the move: from b2, to b2, build c2.
    sq_i from = text_to_square("b2"); // 6
    Moves::Move hermes_move(from, 10, from, Constants::God::HERMES);

    // Generate the text representation of the move.
    std::string move_text = hermes_move.to_text(board);

    // The text for a stand-still move should only contain the start and build squares.
    EXPECT_EQ(move_text, "b2a3b2") << "to_text for a stand-still build should be from_sq + build_sq";
    EXPECT_NE(move_text, "b2b2") << "to_text should use the build square, not the destination square twice";
}
} // namespace Santorini
