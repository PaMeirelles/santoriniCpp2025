#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/board.h"
#include "../src/constants.h"
#include "../src/moves.h"
#include <vector>

namespace Santorini {

    // ############################################################################
    // # Test Generated Moves Are Valid
    // ############################################################################

    TEST(TestGeneratedMovesAreValid, AllMovesAreValid) {
        auto scenarios = get_stress_scenarios();
        for (auto& board : scenarios) {
            auto moves = board.generate_moves();
            for (const auto& move : moves) {
                ASSERT_TRUE(is_move_in_generated_list(board, move))
                    << "Generated move " << Moves::move_to_text(board, move)
                    << " was deemed invalid on board:\n"
                    << board.to_text();
            }
        }
    }

} // namespace Santorini
