#include "gtest/gtest.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include "../test_helpers.h"
#include <vector>
#include <optional>
#include <algorithm>

namespace Santorini {
    TEST(PanTests, pan_win_by_moving_down_two) {
        /**
         * If Pan steps down 2+ levels, that triggers an immediate win.
         */
        // Squares 0 = height 2, 1 = height 0.
        std::array<sq_i, 25> blocks{};
        std::fill(blocks.begin(), blocks.end(), 0);
        blocks[0] = 2;

        // Gray is Pan.
        Board board = create_board(blocks, {0, 2}, {3, 4}, 1, Constants::God::PAN, Constants::God::APOLLO);

        // Move from 0 (height 2) down to 1 (height 0), a difference of -2.
        Moves::Move move = Moves::create_move(0, 1, 5, Constants::God::PAN);
        EXPECT_TRUE(is_move_in_generated_list(board, move)) << "Pan's move should be valid";
        board.make_move(move);

        // Check the board state. Pan is Gray, so a win returns 1.
        EXPECT_EQ(board.check_state(), 1) << "Pan should win by moving down two levels";
    }
};