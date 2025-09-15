#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/transposition_table.h"
#include "../src/board.h"
#include "../src/moves.h"

namespace Santorini {

class TranspositionTableTest : public ::testing::Test {
protected:
    // Create a standard board and TT for use in all tests
    Board board = create_board();
    TranspositionTable tt{1 << 16}; // Use a smaller table for tests
};

TEST_F(TranspositionTableTest, StoreAndProbe) {
    Moves::Move move(0, 5, 6, Constants::God::APOLLO);
    int score = 100;
    int depth = 5;
    char flag = 'E'; // Exact score

    // Store the entry
    tt.store(board, move, score, depth, flag);
    EXPECT_EQ(tt.new_writes, 1);
    EXPECT_EQ(tt.overwrites, 0);

    // Probe and check if we get the same data back
    auto [probed_move, probed_score] = tt.probe(board, 50, 150, depth);

    ASSERT_NE(probed_move, nullptr);
    EXPECT_EQ(probed_move->to_text(board), move.to_text(board));
    ASSERT_TRUE(probed_score.has_value());
    EXPECT_EQ(*probed_score, score);
    EXPECT_EQ(tt.hits, 1);
}

TEST_F(TranspositionTableTest, ClearTable) {
    Moves::Move move(0, 5, 6, Constants::God::APOLLO);
    tt.store(board, move, 100, 5, 'E');
    ASSERT_EQ(tt.new_writes, 1);

    tt.clear();
    EXPECT_EQ(tt.new_writes, 0);
    EXPECT_EQ(tt.overwrites, 0);
    EXPECT_EQ(tt.hits, 0);

    // Probing should now return nothing
    auto [probed_move, probed_score] = tt.probe(board, 50, 150, 5);
    EXPECT_EQ(probed_move, nullptr);
    EXPECT_FALSE(probed_score.has_value());
}

TEST_F(TranspositionTableTest, ProbePVMove) {
    Moves::Move move(0, 5, 6, Constants::God::APOLLO);
    tt.store(board, move, 123, 8, 'E');

    auto [pv_move, pv_score] = tt.probe_pv_move(board);
    ASSERT_NE(pv_move, nullptr);
    EXPECT_EQ(pv_move->to_text(board), move.to_text(board));
    ASSERT_TRUE(pv_score.has_value());
    EXPECT_EQ(*pv_score, 123);
}

TEST_F(TranspositionTableTest, ProbePVLine) {
    Board b1 = create_board();
    Moves::Move move1(0, 5, 6, Constants::God::APOLLO);
    tt.store(b1, move1, 100, 5, 'E');

    Board b2 = create_board();
    b2.make_move(move1);
    Moves::Move move2(23, 18, 17, Constants::God::APOLLO); // Blue's move
    tt.store(b2, move2, -50, 4, 'E');

    Board b3 = create_board();
    b3.make_move(move1);
    b3.make_move(move2);
    Moves::Move move3(5, 4, 3, Constants::God::APOLLO);
    tt.store(b3, move3, 110, 3, 'E');

    // Reconstruct the PV line from the initial board state
    auto pv_line = tt.probe_pv_line(b1);

    ASSERT_EQ(pv_line.size(), 3);
    EXPECT_EQ(pv_line[0].to_text(board), move1.to_text(board));
    EXPECT_EQ(pv_line[1].to_text(board), move2.to_text(board));
    EXPECT_EQ(pv_line[2].to_text(board), move3.to_text(board));
}

} // namespace Santorini
