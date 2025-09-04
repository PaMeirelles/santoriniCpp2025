#include "gtest/gtest.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include "../test_helpers.h"
#include <vector>
#include <optional>
#include <algorithm>

namespace Santorini {

    TEST(MinotaurTests, can_only_push_where_you_can_move) {
        /**
         * If from_sq->to_sq is not a valid move (e.g. 2+ levels up or not adjacent),
         * you can't push the occupant even if occupant is an enemy.
         */
        std::array<sq_i, 25> blocks{};
        std::fill(blocks.begin(), blocks.end(), 0);
        blocks[1] = 2; // too high to climb if from_sq=0 has height=0 => difference=2 => not allowed

        Board board = create_board(blocks,
                                   {0, 2}, {1, 3}, // Gray workers at 0,2; Blue workers at 1,3
                                   1, // Gray's turn
                                   Constants::God::MINOTAUR, Constants::God::APOLLO);

        // Attempt push from 0->1 => that's up 2 levels => invalid
        Moves::Move move(0, 1, 5, Constants::God::MINOTAUR);
        EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Minotaur cannot push to a square that is 2 or more levels higher";
    }

    TEST(MinotaurTests, cannot_push_off_edge) {
        Board board = create_board(std::nullopt, {5, 20}, {0, 24}, 1, Constants::God::MINOTAUR, Constants::God::APOLLO);

        // Gray is at 5. Blue is at 0.
        // Pushing Blue from 0 by moving from 5 is not a valid Minotaur move
        // since `_is_opponent_worker` check fails on a worker at the new square.
        // The correct interpretation of the Python test is to test a specific
        // scenario, so we create a new board with the right worker placement
        // before the invalid move is attempted.

        // We create a new board with Gray at 5 and Blue at 0.
        Board test_board = create_board(std::nullopt, {5, 20}, {0, 24}, 1, Constants::God::MINOTAUR, Constants::God::APOLLO);

        // Gray tries to move from 5->0, which would push the opponent off the board.
        // From 5->0 is a vector (-5, 0). Push square is (0-5) = -5.
        Moves::Move move_fail(5, 0, 1, Constants::God::MINOTAUR);
        EXPECT_FALSE(is_move_in_generated_list(test_board, move_fail)) << "Minotaur cannot push a worker off the board";
    }

    TEST(MinotaurTests, cannot_build_where_you_push) {
        Board board = create_board(std::nullopt, {0, 1}, {2, 4}, 1, Constants::God::MINOTAUR, Constants::God::APOLLO);

        // Minotaur (Gray) at 1 tries to push opponent (Blue) at 2.
        // The push square is 3 (1->2 is vector (1,0), so 2+(1,0) = 3).
        // The build square is also 3. This should be invalid.
        Moves::Move move(1, 2, 3, Constants::God::MINOTAUR);
        EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Minotaur cannot build on the square where the opponent is pushed";
    }

    TEST(MinotaurTests, push_to_3_doesnt_win) {
        std::array<sq_i, 25> blocks{};
        std::fill(blocks.begin(), blocks.end(), 0);
        blocks[2] = 2; // worker at 2 starts on height 2
        blocks[3] = 2; // to_sq is at height 2
        blocks[4] = 3; // pushed square is at height 3

        // Gray worker 0 is at 2. Blue worker 0 is at 3.
        // It's Gray's turn (Minotaur). Gray will move from 2 to 3, pushing Blue to 4.
        Board board = create_board(blocks, {2, 1}, {3, 5}, 1, Constants::God::MINOTAUR, Constants::God::APOLLO);

        // Minotaur moves from 2->3, pushing Blue worker from 3->4.
        // The pushed worker lands on a level 3 block, but this does not count as a win.
        Moves::Move move(2, 3, 2, Constants::God::MINOTAUR);
        move.minotaur_pushed = true; // Manually set flag for test clarity
        EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Move should be valid";
        board.make_move(move);

        // Check if the game state is a win. It should be 0 (no win) because the push does not grant victory.
        EXPECT_EQ(board.check_state(), 0) << "Pushed worker landing on level 3 does not trigger a win";
    }

} // namespace Santorini
