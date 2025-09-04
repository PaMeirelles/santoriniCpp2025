#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/board.h" // Note: Board class must grant friend access to these tests to call private _generate functions.
#include "../src/constants.h"
#include "../src/moves.h"
#include <vector>
#include <array>
#include <string>


namespace Santorini {

TEST(ClimberQuietMoveTests, VerifyPropertiesAndCompleteness) {
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

    int total_configs_tested = 0;

    for (const auto& god : all_gods) {
        for (const auto& blocks : block_configs) {
            for (const auto& workers : worker_sets) {
                // --- Test with Gray active ---
                Board board_gray = create_board(
                    blocks, workers.first, workers.second, 1, god, Constants::God::APOLLO, false
                );

                auto total_moves_gray = board_gray.generate_moves();
                auto climber_moves_gray = board_gray._generate_climber_god_moves();
                auto quiet_moves_gray =board_gray._generate_quiet_god_moves();

                // 1. Check that sum of quiet and climber moves equals total moves
                ASSERT_EQ(climber_moves_gray.size() + quiet_moves_gray.size(), total_moves_gray.size())
                    << "Sum mismatch for Gray as " << static_cast<int>(god)
                    << "\nBoard:\n" << board_gray.to_text();

                // 2. Verify that all climber moves actually go up
                for (const auto& move : climber_moves_gray) {
                    ASSERT_GT(board_gray.get_blocks()[move.to_sq], board_gray.get_blocks()[move.from_sq])
                        << "Quiet move found in climber moves for Gray as " << static_cast<int>(god)
                        << " on move: " << move.to_text(board_gray)
                        << "\nBoard:\n" << board_gray.to_text();
                }

                // 3. Verify that all quiet moves don't go up
                for (const auto& move : quiet_moves_gray) {
                    int from_h = board_gray.get_blocks()[move.from_sq];
                    int to_h = board_gray.get_blocks()[move.to_sq];

                    // For Prometheus pre-build, the height of to_sq is effectively one higher if built upon
                    if (move.god == Constants::God::PROMETHEUS && move.extra_build_sq.has_value() && move.to_sq == *move.extra_build_sq) {
                        to_h++;
                    }

                    ASSERT_LE(to_h, from_h)
                        << "Climber move found in quiet moves for Gray as " << static_cast<int>(god)
                        << " on move: " << move.to_text(board_gray)
                        << "\nBoard:\n" << board_gray.to_text();
                }

                // --- Test with Blue active ---
                Board board_blue = create_board(
                    blocks, workers.first, workers.second, -1, Constants::God::APOLLO, god, false
                );

                auto total_moves_blue = board_blue.generate_moves();
                auto climber_moves_blue = board_blue._generate_climber_god_moves();
                auto quiet_moves_blue =board_blue._generate_quiet_god_moves();

                // 1. Check that sum of quiet and climber moves equals total moves
                ASSERT_EQ(climber_moves_blue.size() + quiet_moves_blue.size(), total_moves_blue.size())
                    << "Sum mismatch for Blue as " << static_cast<int>(god)
                    << "\nBoard:\n" << board_blue.to_text();

                // 2. Verify that all climber moves actually go up
                for (const auto& move : climber_moves_blue) {
                    ASSERT_GT(board_blue.get_blocks()[move.to_sq], board_blue.get_blocks()[move.from_sq])
                        << "Quiet move found in climber moves for Blue as " << static_cast<int>(god)
                        << " on move: " << move.to_text(board_blue)
                        << "\nBoard:\n" << board_blue.to_text();
                }

                // 3. Verify that all quiet moves don't go up
                for (const auto& move : quiet_moves_blue) {
                    int from_h = board_blue.get_blocks()[move.from_sq];
                    int to_h = board_blue.get_blocks()[move.to_sq];

                    if (move.god == Constants::God::PROMETHEUS && move.extra_build_sq.has_value() && move.to_sq == *move.extra_build_sq) {
                        to_h++;
                    }

                    ASSERT_LE(to_h, from_h)
                        << "Climber move found in quiet moves for Blue as " << static_cast<int>(god)
                        << " on move: " << move.to_text(board_blue)
                        << "\nBoard:\n" << board_blue.to_text();
                }
                total_configs_tested++;
            }
        }
    }

    // Final check to ensure we actually ran the tests
    EXPECT_GT(total_configs_tested, 0) << "No configurations were tested.";
}

} // namespace Santorini
