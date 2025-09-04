#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <utility>

#include "../src/board.h"
#include "../src/constants.h"

namespace Santorini {

/**
 * @brief Creates the canonical 54-char position string from board state components.
 */
inline std::string make_position(
    const std::array<sq_i, 25>& blocks,
    const std::pair<sq_i, sq_i>& gray_workers,
    const std::pair<sq_i, sq_i>& blue_workers,
    int turn,
    Constants::God god_gray,
    Constants::God god_blue,
    bool athena_up = false)
{
    // ... implementation from previous step
    std::string pos_str;
    pos_str.reserve(54);
    std::array<char, 25> worker_map{};
    worker_map.fill('N');
    worker_map[gray_workers.first] = 'G';
    worker_map[gray_workers.second] = 'G';
    worker_map[blue_workers.first] = 'B';
    worker_map[blue_workers.second] = 'B';

    for (int i = 0; i < 25; ++i) {
        pos_str += std::to_string(blocks[i]);
        pos_str += worker_map[i];
    }

    pos_str += (turn == 1 ? '0' : '1');
    pos_str += std::to_string(static_cast<int>(god_gray));
    pos_str += std::to_string(static_cast<int>(god_blue));
    pos_str += (athena_up ? '1' : '0');
    return pos_str;
}

/**
 * @brief Creates and returns a Board object with default or custom parameters.
 */
inline Board create_board(
    std::optional<std::array<sq_i, 25>> blocks = std::nullopt,
    std::pair<sq_i, sq_i> gray_workers = {0, 10},
    std::pair<sq_i, sq_i> blue_workers = {23, 24},
    int turn = 1,
    Constants::God god_gray = Constants::God::APOLLO,
    Constants::God god_blue = Constants::God::ARTEMIS,
    bool athena_up = false)
{
    // ... implementation from previous step
    std::array<sq_i, 25> block_data{};
    if (blocks) {
        block_data = *blocks;
    } else {
        block_data.fill(0);
    }

    std::string pos_str = make_position(block_data, gray_workers, blue_workers, turn, god_gray, god_blue, athena_up);
    return Board(pos_str);
}

/**
 * @brief Provides a list of complex board states for stress testing.
 * The definition of this function is in test_hashing.cpp.
 */
std::vector<Board> get_stress_scenarios();


    /**
     * @brief Equality operator to compare two Move objects.
     *
     * This is necessary to find a specific move within a list of generated moves.
     * Two moves are considered equal if their core action properties match, including
     * from, to, build squares, the god performing the move, and any special
     * options like extra builds, domes, or pushes.
     */
    inline bool operator==(const Moves::Move& lhs, const Moves::Move& rhs) {
        return lhs.from_sq == rhs.from_sq &&
               lhs.to_sq == rhs.to_sq &&
               lhs.build_sq == rhs.build_sq &&
               lhs.god == rhs.god &&
               lhs.extra_build_sq == rhs.extra_build_sq &&
               lhs.dome == rhs.dome &&
               lhs.minotaur_pushed == rhs.minotaur_pushed;
    }

    /**
     * @brief Validates a move by checking if it exists in the list of legally
     * generated moves for the current board state.
     *
     * @param board The current board position.
     * @param test_move The move to validate.
     * @return True if the move is found in the list of legal moves, false otherwise.
     */
    inline bool is_move_in_generated_list(const Board& board, const Moves::Move& test_move) {
        auto legal_moves = board.generate_moves();

        for (const auto& legal_move_ptr : legal_moves) {
            if (legal_move_ptr == test_move) {
                return true;
            }
        }

        return false;
    }
} // namespace Santorini
