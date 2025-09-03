#include "gtest/gtest.h"
#include "../test_helpers.h"
#include "../../src/board.h"
#include "../../src/constants.h"
#include "../../src/moves.h"
#include <vector>
#include <optional>

namespace Santorini {

    TEST(ArtemisTests, cannot_move_back_to_where_you_started) {
        /**
         * Artemis gets up to two moves, but cannot return to the original square.
         */
        // Gray=Artemis, Blue=Artemis (Blue god doesn't matter for this test)
        Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::ARTEMIS, Constants::God::ARTEMIS);

        // Attempt from 0->1->0. The 'to_sq' is 0, 'mid_sq' is 1, 'from_sq' is 0.
        // In ArtemisMove constructor: from_sq, to_sq, build_sq, optional<mid_sq>
        Moves::Move move(0, 0, 5, Constants::God::ARTEMIS);
        EXPECT_FALSE(is_move_in_generated_list(board, move));
    }

    TEST(ArtemisTests, can_move_only_once_if_she_wants) {
        /**
         * Artemis can either make a single move or a double move.
         * So from 0->1 with mid_sq=None is also valid.
         */
        // Gray=Artemis, Blue=Artemis (Blue god doesn't matter for this test)
        Board board = create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::ARTEMIS, Constants::God::ARTEMIS);

        // Single step from 0->6, build at 5. No mid_sq provided.
        // In ArtemisMove constructor: from_sq, to_sq, build_sq
        Moves::Move move(0, 6, 5, Constants::God::ARTEMIS);
        EXPECT_TRUE(is_move_in_generated_list(board, move));
    }

} // namespace Santorini