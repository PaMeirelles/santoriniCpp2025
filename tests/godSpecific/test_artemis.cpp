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

    TEST(ArtemisTests, to_text_chooses_valid_path) {
        // This position has a Gray worker on b2. Artemis's to_text() for a move from c3 to b1
        // should not choose b2 as the intermediate square.
        Board board = Board("0N1N0N0N0N1N1G0N0N0N0B0N0G0N0N0N0N0N0N0N0N1N0B0N0N0100");
        using namespace Santorini::Moves;

        // Manually create the move from c3 to b1.
        sq_i from = text_to_square("c3");
        sq_i to = text_to_square("b1");
        sq_i build = text_to_square("a1");
        Moves::Move artemis_move(from, to, build, Constants::God::ARTEMIS);


        // Generate the text representation of the move.
        std::string move_text = artemis_move.to_text(board);
        EXPECT_NE(move_text, "c3b2b1a1") << "to_text should not choose an occupied square (b2) for the path";
    }

    TEST(ArtemisTests, to_text_chooses_valid_path_respecting_height) {
        // In this position, a move from a3 to c3 has two geometric intermediate squares: b3 and b4.
        // However, moving a3(h=0) -> b4(h=2) is an illegal climb.
        // The to_text function must choose the valid path via b3(h=0).
        Board board("0N0N0N1N0N0N1G1N0B0N0B0N2N0N1N1N2N0N0G0N0N0N0N0N0N1010");
        using namespace Santorini::Moves;

        // Manually create the move from a3 to c3, building at d3.
        sq_i from = text_to_square("a3");
        sq_i to = text_to_square("b4");
        sq_i build = text_to_square("c3");
        Moves::Move artemis_move(from, to, build, Constants::God::ARTEMIS);

        // Generate the text representation of the move.
        std::string move_text = artemis_move.to_text(board);

        // Assert that the illegal path via b4 was not chosen.
        EXPECT_NE(move_text, "a3b4c3") << "to_text should not choose an intermediate square that is too high to climb to.";

    }
} // namespace Santorini