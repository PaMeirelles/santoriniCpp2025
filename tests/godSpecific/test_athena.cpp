#include "gtest/gtest.h"
#include "../test_helpers.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include <vector>
#include <optional>
#include <algorithm> // For std::all_of

namespace Santorini {

TEST(AthenaTests, athena_power_works) {
    /**
     * If Athena moves up, the opponent cannot move up on their next turn.
     * Test that scenario explicitly.
     */
    // Let's have Gray=Athena, Blue=Apollo for variety
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    // squares 0 => height=0, 1 => height=1
    blocks[1] = 1;
    Board board = create_board(blocks,
                               {0, 2}, {3, 4}, // Gray workers at 0,2; Blue workers at 3,4
                               1, // Gray's turn
                               Constants::God::ATHENA, Constants::God::APOLLO);

    // Gray moves from 0 (height=0) to 1 (height=1) => that's an "up" move
    Moves::Move move_athena = Moves::create_move(0, 1, 5, Constants::God::ATHENA); // From 0, to 1, build at 5
    EXPECT_TRUE(is_move_in_generated_list(board, move_athena)) << "Athena move should be valid";
    board.make_move(move_athena);

    // After Athena's move, her power should be active.
    // We can access `_prevent_up_next_turn` because the test class is a friend.
    EXPECT_TRUE(board._prevent_up_next_turn) << "Athena's prevent_up_next_turn flag should be true";

    // Now it's Blue's turn (board.turn should be -1 for Blue).
    // They have the Apollo power.
    // Update board blocks for Blue's potential move
    board._blocks[3] = 0; // Blue worker at 3, height 0
    board._blocks[8] = 1; // Adjacent square 8, height 1 (an upward move)

    // Blue tries from 3->8 (adjacent), that's an up move => should fail due to Athena's effect
    Moves::Move move_apollo = Moves::create_move(3, 8, 2, Constants::God::APOLLO); // From 3, to 8, build at 2
    EXPECT_FALSE(is_move_in_generated_list(board, move_apollo)) << "Apollo's upward move should be invalid due to Athena's power";
}
//----------------------------------------------------------------------------------------------------------------------
TEST(AthenaTests, opponent_generated_moves_do_not_climb_after_athena_up) {
    /**
     * After Athena moves up, the opponent should not generate any moves
     * that result in an upward movement.
     */
    // Create a board such that upward moves are normally possible for Blue.
    // Gray will be using Athena and Blue using Apollo.
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[8] = 1; // Potential upward move for Blue from level 0 -> 1
    blocks[1] = 1; // Making the move upward possible for Athena

    // Set up workers: Gray on squares 0 and 2, Blue on squares 3 and 4.
    Board board = create_board(blocks,
                               {0, 2},
                               {3, 4},
                               1,  // Gray's turn
                               Constants::God::ATHENA,
                               Constants::God::APOLLO);

    // Adjust blocks so that an up move is available for Gray as Athena.
    // For example, move Gray's worker from square 0 (at level 0) to square 1.
    Moves::Move move_athena = Moves::create_move(0, 1, 5, Constants::God::ATHENA); // Move 0->1, build 5
    EXPECT_TRUE(is_move_in_generated_list(board, move_athena)) << "Athena move should be valid";
    board.make_move(move_athena);

    // After Athena goes up, the opponent (Blue) now gets the move.
    // Athena's effect should prevent any move that climbs upward.
    std::vector<Moves::Move> blue_moves = board.generate_moves(); // Should generate moves for Blue

    // Check that all generated moves for Blue do not involve moving up
    bool all_moves_are_not_upward = std::all_of(blue_moves.begin(), blue_moves.end(),
        [&](const Moves::Move& move) {
           sq_i origin = Moves::get_from_sq(move.move);
           sq_i final_pos = Moves::get_to_sq(move.move);
            // In C++, we need to access _blocks directly for the board state.
            return board._blocks[final_pos] <= board._blocks[origin];
        }
    );
    EXPECT_TRUE(all_moves_are_not_upward) << "Opponent generated moves should not include upward movements after Athena's power";
}
//----------------------------------------------------------------------------------------------------------------------
TEST(AthenaTests, athena_opponent_no_move_loses) {
    /**
     * If Athena's power forbids the opponent from moving up,
     * and that means the opponent has 0 possible moves, they lose immediately.
     */
    std::array<sq_i, 25> blocks{};
    std::fill(blocks.begin(), blocks.end(), 0);
    blocks[2] = 1; // Square 2 is height 1
    blocks[5] = 1; // Square 5 is height 1
    blocks[6] = 1; // Square 6 is height 1
    blocks[7] = 1; // Square 7 is height 1

    // Gray's turn, but starting with a setup where Blue (Athena) has just moved.
    // This implies that it's currently Gray's turn, but Athena's effect is active.
    // We need to set up the board *as if* Athena just moved up.
    // Let's create a board where Athena (Blue) makes an upward move.
    Board board = create_board(blocks,
                               {0, 1}, {3, 4}, // Gray workers 0,1; Blue workers 3,4
                               -1, // It's currently Blue's turn, Athena is Blue
                               Constants::God::APOLLO, Constants::God::ATHENA);

    // Athena (Blue) moves up from 3 (height 0) to 2 (height 1), building on 8.
    // After this move, it becomes Gray's turn, and Athena's effect should apply.
    Moves::Move move_athena = Moves::create_move(3, 2, 8, Constants::God::ATHENA);
    EXPECT_TRUE(is_move_in_generated_list(board, move_athena)) << "Athena's initial move should be valid";
    board.make_move(move_athena);

    // Now it's Gray's turn. Athena's power is active (`_prevent_up_next_turn` is true).
    // Gray is at 0 (level 0) and 1 (level 0).
    // Adjacent squares:
    // From worker at 0: (1, 5, 6)
    // From worker at 1: (0, 2, 5, 6, 7)
    // Blocks:
    // blocks[1]=0, blocks[5]=1, blocks[6]=1, blocks[2]=1, blocks[7]=1
    // All possible moves for Gray are upward moves to blocks at height 1, or to already occupied squares.
    // Therefore, Gray should have no valid moves.
    EXPECT_EQ(board.check_state(), -1) << "Gray should lose (Blue wins) because Athena's power prevents all valid moves.";
}

} // namespace Santorini

