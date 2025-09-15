#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/board.h"
#include "../src/constants.h"
#include "../src/moves.h"

namespace Santorini {

// ############################################################################
// # Test General/Common Movement Rules
// ############################################################################

TEST(TestGeneralRules, MoveDownAnyNumberOfSquares) {
    std::array<sq_i, 25> blocks{};
    blocks.fill(0);
    blocks[0] = 3;
    Board board = create_board(blocks, {0, 2});

    Moves::Move move(0, 5, 6, Constants::God::APOLLO);
    EXPECT_TRUE(is_move_in_generated_list(board, move));

    board.make_move(move);

    // Check that worker 0 moved from square 0 to 5
    std::string pos_text = board.to_text();
    EXPECT_EQ(pos_text[11], 'G'); // Worker at sq 5
    EXPECT_EQ(pos_text[1], 'N');  // No worker at sq 0

    // Check that a block was built on square 6
    EXPECT_EQ(pos_text[12], '1'); // Block height at sq 6
}

TEST(TestGeneralRules, CanBuildWhereYouCameFrom) {
    Board board = create_board(std::nullopt, {0, 2});
    Moves::Move move(0, 1, 0, Constants::God::APOLLO);

    EXPECT_TRUE(is_move_in_generated_list(board, move));
    board.make_move(move);

    std::string pos_text = board.to_text();
    EXPECT_EQ(pos_text[3], 'G'); // Worker at sq 1
    EXPECT_EQ(pos_text[0], '1'); // Block built at sq 0
}

TEST(TestGeneralRules, CantWarpAroundTheBoard) {
    Board board = create_board();
    Moves::Move move(0, 24, 1, Constants::God::APOLLO);
    EXPECT_FALSE(is_move_in_generated_list(board, move));
}

TEST(TestGeneralRules, CantBuildOnNonAdjacentSquare) {
    Board board = create_board();
    // Move from 1->2, attempt to build on 5.
    // 5 is adjacent to 1, but not to 2.
    Moves::Move move(1, 2, 5, Constants::God::APOLLO);
    EXPECT_FALSE(is_move_in_generated_list(board, move));
}

TEST(TestGeneralRules, NoBuildOnWorker) {
    Board board = create_board(); // Gray workers at 0, 10
    // Move from 0 -> 5, attempt to build on 10 (occupied by other gray worker)
    Moves::Move move(0, 5, 10, Constants::God::APOLLO);
    EXPECT_FALSE(is_move_in_generated_list(board, move));
}

} // namespace Santorini
