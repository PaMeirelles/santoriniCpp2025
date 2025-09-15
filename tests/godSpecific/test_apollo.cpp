#include "gtest/gtest.h"
#include "../test_helpers.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include <vector>

namespace Santorini {

TEST(ApolloTests, swap_up_one_height) {
    /*
    Apollo can move into an opponent's space if it's an adjacent square with an occupant.
    Check that we can do so if the occupant is on a block that's 1 higher or any lower.
    */
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[1] = 1;  // square 1 is height=1

    // Gray at 0, Blue at 1. Gray's turn, Gray=Apollo, Blue=Apollo
    Board board = create_board(blocks, {0, 2}, {1, 3}, 1, Constants::God::APOLLO, Constants::God::APOLLO);

    // from 0->1, occupant is Blue. That occupant is on height=1, which is only 1 higher than height=0 => valid
    // build on 2 for example
    Moves::Move move = Moves::create_move(0, 1, 6, Constants::God::APOLLO);
    EXPECT_TRUE(is_move_in_generated_list(board, move));
    board.make_move(move);
    // They swap, so Blue's worker is now on 0, Gray's worker on 1
    std::array<sq_i, 4> expected_workers = {1, 2, 0, 3};
    EXPECT_EQ(board._workers, expected_workers);
    EXPECT_EQ(board._blocks[6], 1); // built +1
}

TEST(ApolloTests, can_only_swap_with_enemy) {
    /*
    Apollo cannot swap with your own worker.
    So if occupant is an ally, it must fail.
    */
    // Gray at 0,1; Blue at 23,24. Gray tries to swap with square=1 => occupant is Gray => invalid.
    Board board = create_board(std::nullopt, {0, 1}, {23, 24}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    Moves::Move move = Moves::create_move(0, 1, 2, Constants::God::APOLLO);
    EXPECT_FALSE(is_move_in_generated_list(board, move));
}

TEST(ApolloTests, no_moves_but_apollo_swap_saves_you) {
    // We'll create a scenario where Gray is on square 0 with height=2,
    // and every adjacent square is height=4 except square 1 which is an enemy occupant.
    // Because of Apollo, Gray can swap with the occupant on 1 (assuming 1 is not dome).
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 4);
    // Let's say we set square 0 = 2, square 1 = 2 (not a dome) but occupant is Blue
    blocks[0] = 2;
    blocks[1] = 2;
    blocks[2] = 2;
    blocks[3] = 2;
    blocks[7] = 3;

    // Gray at 0, second Gray worker at 2 (irrelevant?), Blue at 1,3
    Board board = create_board(blocks, {0, 2}, {1, 3}, 1, Constants::God::APOLLO, Constants::God::APOLLO);

    // If not for Apollo swap, everything else is 4 => no moves. But we do have an Apollo swap with occupant on 1.
    // We'll do from_sq=0 -> to_sq=1, build at square=2 for instance.
    Moves::Move move = Moves::create_move(0, 1, 7, Constants::God::APOLLO);
    EXPECT_TRUE(is_move_in_generated_list(board, move));  // This means we are not forced-losing.
    // We can even apply the move
    board.make_move(move);
    EXPECT_EQ(board._workers[0], 1);  // Gray's worker on 1
    EXPECT_EQ(board._workers[2], 0);  // Blue's worker swapped onto 0
}

TEST(ApolloTests, no_build_on_from_when_swapping) {
    Board board = create_board(std::nullopt, {0, 1}, {2, 3});
    Moves::Move move = Moves::create_move(1, 2, 1, Constants::God::APOLLO);
    EXPECT_FALSE(is_move_in_generated_list(board, move));
}

TEST(ApolloTests, no_swap_can_build) {
    std::array<sq_i, 25> blocks = {
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0
    };
    Board board = create_board(blocks, {7, 15}, {0, 2}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    Moves::Move move = Moves::create_move(7, 3, 7, Constants::God::APOLLO);
    EXPECT_TRUE(is_move_in_generated_list(board, move));
}

TEST(ApolloTests, misc) {
    Board board("2N1N1N1N1N2N4N4N4N4N1N3N1N4N0G3N4N1N4N1G2N1B4N4N1B0060");
    int state = board.check_state();
    EXPECT_EQ(state, -1);
}

} // namespace Santorini