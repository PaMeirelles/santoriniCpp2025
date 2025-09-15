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
                    auto new_h = board_gray.get_blocks()[Moves::get_to_sq(move.move)];
                    auto prev_h = board_gray.get_blocks()[Moves::get_from_sq(move.move)];
                    if (Moves::get_god(move.move) == Constants::God::PAN && prev_h >= 2 && new_h == 0) continue;
                    ASSERT_GT(new_h, prev_h)
                        << "Quiet move found in climber moves for Gray as " << static_cast<int>(god)
                        << " on move: " << Moves::move_to_text(board_gray, move)
                        << "\nBoard:\n" << board_gray.to_text();
                }

                // 3. Verify that all quiet moves don't go up
                for (const auto& move : quiet_moves_gray) {
                    int from_h = board_gray.get_blocks()[Moves::get_from_sq(move.move)];
                    int to_h = board_gray.get_blocks()[Moves::get_to_sq(move.move)];
                    if (Moves::get_god(move.move) == Constants::God::PAN && from_h >= 2 && to_h == 0) continue;

                    // For Prometheus pre-build, the height of to_sq is effectively one higher if built upon
                    if (Moves::get_god(move.move) == Constants::God::PROMETHEUS &&
                        Moves::get_extra_build_sq(move.move).has_value() &&
                        Moves::get_to_sq(move.move) == *Moves::get_extra_build_sq(move.move)) {
                        to_h++;
                    }

                    ASSERT_LE(to_h, from_h)
                        << "Climber move found in quiet moves for Gray as " << static_cast<int>(god)
                        << " on move: " << move_to_text(board_gray, move)
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
                    if (Moves::get_god(move.move) == Constants::God::PAN &&
                        board_blue.get_blocks()[Moves::get_from_sq(move.move)] >= 2 &&
                        board_blue.get_blocks()[Moves::get_to_sq(move.move)] == 0) continue;
                    ASSERT_GT(board_blue.get_blocks()[Moves::get_to_sq(move.move)], board_blue.get_blocks()[Moves::get_from_sq(move.move)])
                        << "Quiet move found in climber moves for Blue as " << static_cast<int>(god)
                        << " on move: " << move_to_text(board_blue, move)
                        << "\nBoard:\n" << board_blue.to_text();
                }

                // 3. Verify that all quiet moves don't go up
                for (const auto& move : quiet_moves_blue) {
                    int from_h = board_blue.get_blocks()[Moves::get_from_sq(move.move)];
                    int to_h = board_blue.get_blocks()[Moves::get_to_sq(move.move)];
                    if (Moves::get_god(move.move) == Constants::God::PAN && from_h >= 2 && to_h == 0) continue;

                    if (Moves::get_god(move.move) == Constants::God::PROMETHEUS && Moves::get_extra_build_sq(move.move).has_value() && Moves::get_to_sq(move.move) == *Moves::get_extra_build_sq(move.move)) {
                        to_h++;
                    }

                    ASSERT_LE(to_h, from_h)
                        << "Climber move found in quiet moves for Blue as " << static_cast<int>(god)
                        << " on move: " << move_to_text(board_blue, move)
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
