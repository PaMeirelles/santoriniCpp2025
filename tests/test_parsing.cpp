#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/board.h"
#include "../src/constants.h"
#include "../src/moves.h"

namespace Santorini {

// ############################################################################
// # Test Position Parsing
// ############################################################################

TEST(TestPositionParsing, ValidParsing) {
    // Test case 1: Standard starting position
    std::array<sq_i, 25> blocks1{};
    blocks1.fill(0);
    std::string pos_str1 = make_position(blocks1, {0, 1}, {23, 24}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    Board board1(pos_str1);
    EXPECT_EQ(board1.to_text(), pos_str1);

    // Test case 2: Blue's turn with different gods and block heights
    std::array<sq_i, 25> blocks2{};
    blocks2.fill(1);
    blocks2[0] = 2; blocks2[1] = 2; blocks2[2] = 0; blocks2[3] = 4;
    std::string pos_str2 = make_position(blocks2, {0, 1}, {2, 3}, -1, Constants::God::PAN, Constants::God::PROMETHEUS);
    Board board2(pos_str2);
    EXPECT_EQ(board2.to_text(), pos_str2);
}

TEST(TestPositionParsing, InvalidLength) {
    EXPECT_THROW({ Board b("too_short"); }, std::invalid_argument);
    EXPECT_THROW({ Board b("this_string_is_exactly_55_characters_long_and_invalid"); }, std::invalid_argument);
}

TEST(TestPositionParsing, InvalidBlockHeights) {
    std::array<sq_i, 25> bad_blocks{};
    bad_blocks.fill(0);
    bad_blocks[24] = 9; // Invalid height
    std::string pos_str = make_position(bad_blocks, {0, 1}, {2, 3}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    EXPECT_THROW({ Board b(pos_str); }, std::invalid_argument);
}

TEST(TestPositionParsing, InvalidWorkerCodes) {
    std::array<sq_i, 25> blocks{};
    blocks.fill(0);
    std::string base_str = make_position(blocks, {0, 1}, {2, 3}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    base_str[1] = 'X'; // Inject an invalid worker code
    EXPECT_THROW({ Board b(base_str); }, std::invalid_argument);
}

TEST(TestPositionParsing, InvalidWorkerCount) {
    // Create a position string with 3 Gray workers
    std::string bad_pos_str = "0G0G0G0B0B" + std::string(40, 'N') + "0010";
    EXPECT_THROW({ Board b(bad_pos_str); }, std::invalid_argument);
}

TEST(TestPositionParsing, InvalidTurnChar) {
    std::array<sq_i, 25> blocks{};
    blocks.fill(0);
    std::string pos_str = make_position(blocks, {0, 1}, {2, 3}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    pos_str[50] = '5'; // Invalid turn character
    EXPECT_THROW({ Board b(pos_str); }, std::invalid_argument);
}

TEST(TestPositionParsing, InvalidGodChar) {
    std::array<sq_i, 25> blocks{};
    blocks.fill(0);
    std::string pos_str = make_position(blocks, {0, 1}, {2, 3}, 1, Constants::God::APOLLO, Constants::God::ARTEMIS);
    pos_str[51] = 'X'; // Invalid god characterC
    EXPECT_THROW({ Board b(pos_str); }, std::invalid_argument);
}

TEST(TestPositionParsing, AthenaFlagParsed) {
    std::array<sq_i, 25> blocks{};
    blocks.fill(0);
    blocks[4] = 1; // Blue needs a square to move up to
    Board board = create_board(blocks, {0, 1}, {2, 3}, -1, Constants::God::ATHENA, Constants::God::APOLLO, true);

    Moves::Move move(3, 4, 8, Constants::God::APOLLO);
    EXPECT_FALSE(is_move_in_generated_list(board, move));
}

} // namespace Santorini
