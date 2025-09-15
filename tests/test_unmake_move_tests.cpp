#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/board.h"
#include "../src/constants.h"
#include "../src/moves.h"
#include <vector>
#include <optional>
#include <algorithm>

namespace Santorini {

TEST(FullUnmakeMoveTests, MakeUnmakeRestoresBoardStateAndHash) {
    std::vector<Constants::God> all_gods = {
        Constants::God::APOLLO,
        Constants::God::ARTEMIS,
        Constants::God::ATHENA,
        Constants::God::ATLAS,
        Constants::God::DEMETER,
        Constants::God::HEPHAESTUS,
        Constants::God::HERMES,
        Constants::God::MINOTAUR,
        Constants::God::PAN,
        Constants::God::PROMETHEUS
    };

    std::vector<std::array<sq_i, 25>> block_configs;
    std::array<sq_i, 25> base_blocks1{};
    base_blocks1.fill(0);
    block_configs.push_back(base_blocks1);

    std::array<sq_i, 25> base_blocks2 = {
        0, 1, 2, 0, 0,
        1, 2, 3, 0, 0,
        0, 0, 1, 1, 0,
        0, 2, 2, 1, 0,
        0, 0, 0, 1, 0
    };
    block_configs.push_back(base_blocks2);

    std::array<sq_i, 25> base_blocks3 = {
        3, 2, 1, 0, 0,
        0, 1, 2, 3, 0,
        0, 0, 0, 0, 0,
        1, 2, 3, 4, 0,
        0, 0, 1, 2, 3
    };
    block_configs.push_back(base_blocks3);

    std::vector<std::pair<std::pair<sq_i, sq_i>, std::pair<sq_i, sq_i>>> worker_sets = {
        {{0, 1}, {23, 24}},
        {{5, 7}, {12, 14}},
        {{2, 10}, {15, 24}},
        {{0, 12}, {8, 20}}
    };

    int total_moves_tested = 0;

    for (const auto& god : all_gods) {
        for (const auto& blocks : block_configs) {
            for (const auto& workers : worker_sets) {
                // Test with Gray active
                Board board_gray = create_board(
                    blocks, workers.first, workers.second, 1, god, Constants::God::APOLLO, false
                );
                uint64_t start_hash_gray = board_gray.get_hash();
                std::string start_pos_gray = board_gray.to_text();

                auto moves_gray = board_gray.generate_moves();
                for (const auto& move : moves_gray) {
                    board_gray.make_move(move);
                    board_gray.unmake_move(move);
                    ASSERT_EQ(board_gray.get_hash(), start_hash_gray)
                        << "Hash mismatch after make/unmake for Gray as " << static_cast<int>(god)
                        << " on move: " << Moves::move_to_text(board_gray, move)
                        << "\nInitial board:\n" << start_pos_gray;
                    ASSERT_EQ(board_gray.to_text(), start_pos_gray)
                        << "Board state mismatch after make/unmake for Gray as " << static_cast<int>(god)
                        << " on move: " << Moves::move_to_text(board_gray, move)
                        << "\nBefore: " << start_pos_gray
                        << "\nAfter:  " << board_gray.to_text();
                    total_moves_tested++;
                }

                // Test with Blue active
                Board board_blue = create_board(
                    blocks, workers.first, workers.second, -1, Constants::God::APOLLO, god, false
                );
                uint64_t start_hash_blue = board_blue.get_hash();
                std::string start_pos_blue = board_blue.to_text();

                auto moves_blue = board_blue.generate_moves();
                for (const auto& move : moves_blue) {
                    board_blue.make_move(move);
                    board_blue.unmake_move(move);
                    ASSERT_EQ(board_blue.get_hash(), start_hash_blue)
                        << "Hash mismatch after make/unmake for Blue as " << static_cast<int>(god)
                        << " on move: " << Moves::move_to_text(board_blue, move)
                        << "\nInitial board:\n" << start_pos_blue;
                    ASSERT_EQ(board_blue.to_text(), start_pos_blue)
                        << "Board state mismatch after make/unmake for Blue as " << static_cast<int>(god)
                        << " on move: " << Moves::move_to_text(board_blue, move)
                        << "\nBefore: " << start_pos_blue
                        << "\nAfter:  " << board_blue.to_text();
                    total_moves_tested++;
                }
            }
        }
    }
    
    // Final check to ensure we actually ran the tests
    EXPECT_GT(total_moves_tested, 0) << "No moves were generated or tested.";
}

} // namespace Santorini