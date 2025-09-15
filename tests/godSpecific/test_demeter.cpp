#include "gtest/gtest.h"
#include "../test_helpers.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include <vector>
#include <optional>

namespace Santorini {



TEST(DemeterTests, cannot_build_twice_on_same_square) {
    /**
     * Demeter can build twice, but NOT on the same square.
     */
    // Gray=Demeter
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::DEMETER, Constants::God::ARTEMIS);

    // Attempt from 0->1, build_sq_1=2, build_sq_2=2 => invalid
    // DemeterMove constructor:sqfrom,sqto,sqb1, optional<sq> b2 = nullopt
    Moves::Move move(0, 1, 2, Constants::God::DEMETER); // Building on square 2 twice
    move.extra_build_sq = 2;
    EXPECT_FALSE(is_move_in_generated_list(board, move)) << "Demeter should not be able to build twice on the same square";
}

//----------------------------------------------------------------------------------------------------------------------

TEST(DemeterTests, can_build_only_once_if_desired) {
    /**
     * Demeter's second build is optional (build_sq_2 can be None).
     */
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::DEMETER, Constants::God::ARTEMIS);

    // From 0->1, build at 2 only once. No second build square.
    // DemeterMove constructor:sqfrom,sqto,sqb1, optional<sq> b2 = nullopt
    Moves::Move move(0, 1, 2, Constants::God::DEMETER); // Building on square 2 twice
    EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Demeter should be able to build only once if desired";
    
    board.make_move(move);
    // Accessing _blocks directly for testing purposes due to friendship
    EXPECT_EQ(board._blocks[2], 1) << "Square 2 should have a height of 1 after Demeter's single build";
}

TEST(DemeterTests, can_build_twice_if_desired) {
    Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::DEMETER, Constants::God::ARTEMIS);

    // From 0->1, build at 2 only once. No second build square.
    // DemeterMove constructor:sqfrom,sqto,sqb1, optional<sq> b2 = nullopt
    Moves::Move move(0, 1, 2, Constants::God::DEMETER);
    move.extra_build_sq = 5;
    EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Demeter should be able to build only once if desired";
}

} // namespace Santorini