#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/evaluation.h"
#include "../src/board.h"
#include "../src/constants.h"

namespace Santorini {

// ############################################################################
// # Test Evaluation Logic
// ############################################################################

TEST(TestEvaluation, HeightAdvantage) {
    // Board 1: Gray worker at height 0
    std::array<sq_i, 25> blocks1{};
    blocks1.fill(0);
    Board b1 = create_board(blocks1, {0, 10});
    int score1 = score_position(b1);

    // Board 2: Same position, but gray worker is now at height 2
    std::array<sq_i, 25> blocks2{};
    blocks2.fill(0);
    blocks2[0] = 2;
    Board b2 = create_board(blocks2, {0, 10});
    int score2 = score_position(b2);

    // A worker on a higher block should have a better score.
    EXPECT_GT(score2, score1);
}

TEST(TestEvaluation, CentralityAdvantage) {
    // Board 1: Gray worker on the edge (square 0)
    Board b1 = create_board(std::nullopt, {0, 10});
    int score1 = score_position(b1);

    // Board 2: Same position, but gray worker is in the center (square 12)
    Board b2 = create_board(std::nullopt, {12, 10});
    int score2 = score_position(b2);

    // A worker in the center is generally better than one on the edge.
    EXPECT_GT(score2, score1);
}

TEST(TestEvaluation, TempoAdvantage) {
    // Board 1: Gray's turn
    Board b1 = create_board();
    int score1 = score_position(b1);

    // Board 2: Same position, but it's Blue's turn
    Board b2 = create_board(std::nullopt, {0, 10}, {23, 24}, -1);
    int score2 = score_position(b2);

    // The score should be higher when it's Gray's turn due to the tempo bonus.
    EXPECT_GT(score1, score2);
}

TEST(TestEvaluation, SupportAdvantage) {
    // Board 1: Worker at height 1, but its neighbors are all at height 0.
    // This position is high, but has poor support for climbing higher.
    std::array<sq_i, 25> blocks1{};
    blocks1.fill(0);
    blocks1[12] = 1; // Worker is in the center at height 1
    Board b1 = create_board(blocks1, {12, 10});
    int score1 = score_position(b1);

    // Board 2: Worker at height 1, and two neighbors are also at height 1.
    // This is a much better position for building and climbing.
    std::array<sq_i, 25> blocks2{};
    blocks2.fill(0);
    blocks2[12] = 1; // Worker
    blocks2[11] = 1; // Neighbor at same height
    blocks2[13] = 1; // Neighbor at same height
    Board b2 = create_board(blocks2, {12, 10});
    int score2 = score_position(b2);

    // A worker with same-level neighbors (better support) should score higher.
    EXPECT_GT(score2, score1);
}

TEST(TestEvaluation, NextHeightSupportIsGood) {
    // Board 1: Worker at height 1, surrounded by other height 1 blocks.
    std::array<sq_i, 25> blocks1{};
    blocks1.fill(1);
    Board b1 = create_board(blocks1, {12, 10});
    int score1 = score_position(b1);

    // Board 2: Worker at height 1, but surrounded by height 2 blocks (potential winning moves).
    std::array<sq_i, 25> blocks2{};
    blocks2[12] = 1; // Worker is on a level 1 block
    blocks2[13] = 2;
    blocks2[11] = 1;
    blocks2[7] = 2;
    blocks2[6] = 2;
    blocks2[8] = 2;
    blocks2[16] = 2;
    blocks2[17] = 2;
    blocks2[18] = 2;
    Board b2 = create_board(blocks2, {12, 10});
    int score2 = score_position(b2);

    // Being next to level 2 blocks is generally better than being next to level 1 blocks.
    EXPECT_GT(score2, score1);
}


} // namespace Santorini
