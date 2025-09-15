#include "../test_helpers.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include "gtest/gtest.h"
#include <optional>
#include <algorithm>

namespace Santorini {

    TEST(AtlasTests, no_one_moves_on_domes) {
        /**
         * Once a square has block=4 (dome), no one can move there anymore.
         * Check that the board disallows it for any standard move.
         */
        // Gray=Atlas. We'll do a normal Atlas move but test that after building a dome,
        // next turn the opponent can't move onto that dome.
        std::array<sq_i, 25> blocks{};
        std::fill(blocks.begin(), blocks.end(), 0);

        Board board = create_board(blocks,
                                   {0, 2}, {1, 3}, // Gray workers at 0,2; Blue workers at 1,3
                                   1, // Gray's turn
                                   Constants::God::ATLAS, Constants::God::APOLLO);

        // Gray uses Atlas power to move from 0->5, then build a dome at 6
        Moves::Move move_atlas(0, 5, 6, Constants::God::ATLAS);
        move_atlas.dome = true;
        move_atlas.original_height = board._blocks[6]; // Store original height for undo
        EXPECT_TRUE(is_move_in_generated_list(board, move_atlas)) << "Atlas move to build a dome should be valid";
        board.make_move(move_atlas);

        // Verify the dome was built
        EXPECT_EQ(board._blocks[6], 4) << "Square 6 should be a dome (height 4)";

        // Now it's Blue's turn (board.turn should be -1 for Blue).
        // Blue tries to move onto square 6 (the dome) => should be invalid
        Moves::Move move_blue(1, 6, 2, Constants::God::APOLLO); // Blue worker at 1 tries to move to 6, build at 2
        EXPECT_FALSE(is_move_in_generated_list(board, move_blue)) << "Opponent should not be able to move onto a dome";
    }

} // namespace Santorini
