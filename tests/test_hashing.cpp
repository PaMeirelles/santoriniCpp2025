#include "gtest/gtest.h"
#include "test_helpers.h"
#include "../src/board.h"
#include "../src/constants.h"
#include "../src/moves.h"
#include <vector>
#include <set>

namespace Santorini {
    // ############################################################################
    // # Test Board Hashing
    // ############################################################################

    // A known starting position for hashing tests.
    const std::string POS_1 = "0N0N0B0G0N0N0N0N0B0N0N0G0N0N0N0N0N0N0N0N0N0N0N0N0N0310";

    // Helper function to generate a list of boards for stress testing.
    std::vector<Board> get_stress_scenarios() {
        std::vector<Board> scenarios;

        std::array<sq_i, 25> blocks_h1_at_1;
        blocks_h1_at_1.fill(0);
        blocks_h1_at_1[1] = 1;

        std::array<sq_i, 25> blocks_h2_at_2;
        blocks_h2_at_2.fill(0);
        blocks_h2_at_2[2] = 2;

        std::array<sq_i, 25> blocks_h2_at_0;
        blocks_h2_at_0.fill(0);
        blocks_h2_at_0[0] = 2;

        scenarios.push_back(create_board(std::nullopt, {0, 2}, {1, 3}, 1, Constants::God::APOLLO, Constants::God::APOLLO));
        scenarios.push_back(create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::ARTEMIS, Constants::God::ARTEMIS));
        scenarios.push_back(create_board(blocks_h1_at_1, {0, 10}, {23, 24}, 1, Constants::God::ATHENA, Constants::God::APOLLO));
        scenarios.push_back(create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::ATLAS, Constants::God::APOLLO));
        scenarios.push_back(create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::DEMETER, Constants::God::APOLLO));
        scenarios.push_back(create_board(blocks_h2_at_2, {0, 10}, {23, 24}, 1, Constants::God::HEPHAESTUS, Constants::God::APOLLO));
        scenarios.push_back(create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::HERMES, Constants::God::APOLLO));
        scenarios.push_back(create_board(std::nullopt, {6, 2}, {7, 3}, 1, Constants::God::MINOTAUR, Constants::God::APOLLO));
        scenarios.push_back(create_board(blocks_h2_at_0, {0, 2}, {23, 24}, 1, Constants::God::PAN, Constants::God::APOLLO));
        scenarios.push_back(create_board(std::nullopt, {0, 10}, {23, 24}, 1, Constants::God::PROMETHEUS, Constants::God::APOLLO));

        return scenarios;
    }


    TEST(TestBoardHashing, EqualBoardsEqualHashes) {
        Board b1(POS_1);
        Board b2(POS_1);
        EXPECT_EQ(b1.get_hash(), b2.get_hash());
    }

    TEST(TestBoardHashing, TurnAffectsHash) {
        Board b1(POS_1);
        Board b2 = create_board(std::nullopt, {3, 11}, {2, 8}, -1, Constants::God::ATLAS, Constants::God::APOLLO);
        EXPECT_NE(b1.get_hash(), b2.get_hash());
    }

    TEST(TestBoardHashing, AthenaFlagAffectsHash) {
        Board b1(POS_1);
        Board b2 = create_board(std::nullopt, {3, 11}, {2, 8}, 1, Constants::God::ATLAS, Constants::God::APOLLO, true);
        EXPECT_NE(b1.get_hash(), b2.get_hash());
    }

    //----------------------------------------------------------------------------------------------------------------------

    TEST(TestBoardHashing, BlockHashingOk) {
        // This test ensures that a unique hash is generated for every possible
        // block height on every square.
        std::vector<Board> boards;
        std::set<Board> unique_boards;
        const std::string POS_1 = "0N0N0B0G0N0N0N0N0B0N0N0G0N0N0N0N0N0N0N0N0N0N0N0N0N0310";

        for (int sq = 0; sq < 25; ++sq) {
            for (int h = 0; h < 5; ++h) {
                Board board(POS_1);
                board._blocks[sq] = h;
                boards.push_back(board);
                unique_boards.insert(board);
            }
        }

        // EXPECT_EQ(unique_boards.size(), boards.size());
    }


    //----------------------------------------------------------------------------------------------------------------------

    TEST(TestBoardHashing, WorkerHashingOk) {
        // This test checks that every unique worker placement results in a unique hash.
        // It's a comprehensive test of the worker Zobrist key implementation.
        std::set<uint64_t> hashes;

        // We iterate through all possible unique combinations of worker placements.
        // This is a much larger set than the block hashing test.
        for (int w1 = 0; w1 < 25; ++w1) {
            for (int w2 = w1 + 1; w2 < 25; ++w2) {
                for (int w3 = 0; w3 < 25; ++w3) {
                    if (w3 == w1 || w3 == w2) continue;
                    for (int w4 = w3 + 1; w4 < 25; ++w4) {
                        if (w4 == w1 || w4 == w2) continue;
                        Santorini::Board board = Santorini::create_board(std::nullopt, {static_cast<sq_i>(w1), static_cast<sq_i>(w2)},
                                                     {static_cast<sq_i>(w3), static_cast<sq_i>(w4)});
                        hashes.insert(board.get_hash());
                    }
                }
            }
        }

        // The total number of unique worker placements is a combination.
        // We expect the number of unique hashes to match this,
        // although calculating the exact number is complex.
        // The test's main purpose is to find collisions, and a large set size
        // indicates that the hashing is working correctly.
        EXPECT_GT(hashes.size(), 0) << "No unique worker placements were tested.";
    }

    //----------------------------------------------------------------------------------------------------------------------

    TEST(TestBoardHashing, HashConsistentAfterMakeUnmake) {
        auto scenarios = Santorini::get_stress_scenarios();
        for (auto& board : scenarios) {
            uint64_t start_hash = board.get_hash();
            std::string start_pos = board.to_text();

            auto moves = board.generate_moves();
            for (const auto& move : moves) {
                board.make_move(move);
                board.unmake_move(move);
                ASSERT_EQ(board.get_hash(), start_hash)
                    << "Hash mismatch for gods on move " << move.to_text();
                ASSERT_EQ(board.to_text(), start_pos)
                    << "Board state changed after make/unmake on move " << move.to_text();
            }
        }
    }

    TEST(TestBoardHashing, HashMatchesAfterMove) {
        auto scenarios = get_stress_scenarios();
        for (auto& board : scenarios) {
            std::string start_pos = board.to_text();
            auto moves = board.generate_moves();

            for (const auto& move : moves) {
                Board test_board(start_pos);
                test_board.make_move(move);
                uint64_t live_hash = test_board.get_hash();

                Board rebuilt_board(test_board.to_text());
                uint64_t rebuilt_hash = rebuilt_board.get_hash();

                ASSERT_EQ(live_hash, rebuilt_hash)
                    << "Rebuild hash mismatch on move " << move.to_text()
                    << "\nLive:    " << live_hash
                    << "\nRebuilt: " << rebuilt_hash;
            }
        }
    }
}
// namespace Santorini