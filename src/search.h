#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <random>

#include "board.h"
#include "moves.h"
#include "evaluation.h"

namespace Santorini {

// Use the correct type alias as defined in your constants/moves headers
using sq_i = int8_t;

constexpr int MATE = 10000;
constexpr int CHECK_EVERY = 4096;

inline bool is_mate(int score) {
    return score > (MATE - 100) || score < (-MATE + 100);
}

struct SearchResult {
    std::unique_ptr<Moves::Move> best_move = nullptr;
    int score = 0;
    long nodes = 0;
};

inline int evaluate(const Board& board) {
    return score_position(board) * board.get_turn();
}

// =========================================================================================
// MCTS IMPLEMENTATION
// =========================================================================================

struct MCTSNode {
    std::unique_ptr<Moves::Move> move;
    MCTSNode* parent;
    std::vector<std::unique_ptr<MCTSNode>> children;
    std::vector<Moves::Move> untried_moves;

    int visits = 0;
    double score = 0.0;
    int player_to_move; // The ID of the player to move AT THIS state

    MCTSNode(std::unique_ptr<Moves::Move> m, MCTSNode* p, Board& b)
        : move(std::move(m)), parent(p), player_to_move(b.get_turn()) {

        auto climbers = b.generate_climber_moves();
        auto quiets = b.generate_quiet_moves();

        untried_moves.reserve(climbers.size() + quiets.size());
        untried_moves.insert(untried_moves.end(), climbers.begin(), climbers.end());
        untried_moves.insert(untried_moves.end(), quiets.begin(), quiets.end());
    }

    bool is_fully_expanded() const {
        return untried_moves.empty();
    }

    bool is_terminal() const {
        return is_fully_expanded() && children.empty();
    }

    MCTSNode* get_best_uct_child(double exploration_param = 1.414) const {
        MCTSNode* best_child = nullptr;
        double best_uct = -std::numeric_limits<double>::max();

        for (const auto& child : children) {
            if (child->visits == 0) continue;

            // Score represents wins from the perspective of the player making the decision
            // (the player_to_move of the current node).
            double win_rate = child->score / child->visits;
            double uct = win_rate + exploration_param * std::sqrt(std::log(visits) / child->visits);

            if (uct > best_uct) {
                best_uct = uct;
                best_child = child.get();
            }
        }
        return best_child;
    }
};

// Pure random rollout simulation
inline int simulate(Board board) {
    static thread_local std::mt19937 rng(std::random_device{}());
    int depth = 0;
    const int MAX_ROLLOUT_DEPTH = 50;

    while (depth < MAX_ROLLOUT_DEPTH) {
        auto climbers = board.generate_climber_moves();
        for (const auto& m : climbers) {
            if (m.winning) {
                return board.get_turn(); // The player who just moved wins
            }
        }

        auto quiets = board.generate_quiet_moves();
        if (climbers.empty() && quiets.empty()) {
            return -board.get_turn(); // No moves left, current player loses
        }

        std::vector<Moves::Move> all_moves;
        all_moves.reserve(climbers.size() + quiets.size());
        all_moves.insert(all_moves.end(), climbers.begin(), climbers.end());
        all_moves.insert(all_moves.end(), quiets.begin(), quiets.end());

        std::uniform_int_distribution<size_t> dist(0, all_moves.size() - 1);
        board.make_move(all_moves[dist(rng)]);
        depth++;
    }

    // Depth limit reached, use heuristic evaluation to pick a winner
    int eval = evaluate(board); // positive means current player is better off
    if (eval > 0) return board.get_turn();
    if (eval < 0) return -board.get_turn();
    return 0; // Draw
}

inline SearchResult get_best_move(
    Board& board,
    int remaining_time_ms,
    std::optional<int> max_depth = std::nullopt)
{
    // max_depth is repurposed here as a max_iterations scalar if provided
    long max_iterations = max_depth.has_value() ? static_cast<long>(max_depth.value()) * 1000 : std::numeric_limits<long>::max();

    auto start_time = std::chrono::high_resolution_clock::now();
    auto thinking_time = std::chrono::milliseconds(remaining_time_ms / 10);
    auto end_time = start_time + thinking_time;

    static thread_local std::mt19937 rng(std::random_device{}());

    MCTSNode root(nullptr, nullptr, board);
    long nodes_searched = 0;

    while (std::chrono::high_resolution_clock::now() < end_time && nodes_searched < max_iterations) {
        MCTSNode* node = &root;
        Board sim_board = board;

        // 1. Selection
        while (node->is_fully_expanded() && !node->is_terminal()) {
            node = node->get_best_uct_child();
            sim_board.make_move(*(node->move));
        }

        // 2. Expansion
        if (!node->is_fully_expanded()) {
            std::uniform_int_distribution<size_t> dist(0, node->untried_moves.size() - 1);
            size_t move_idx = dist(rng);
            auto move_to_try = node->untried_moves[move_idx];

            // Fast pop-and-swap to remove the selected move
            node->untried_moves[move_idx] = node->untried_moves.back();
            node->untried_moves.pop_back();

            sim_board.make_move(move_to_try);
            auto new_node = std::make_unique<MCTSNode>(std::make_unique<Moves::Move>(move_to_try), node, sim_board);
            node->children.push_back(std::move(new_node));
            node = node->children.back().get();
        }

        // 3. Simulation
        // Returns absolute turn ID of the winning player (e.g. 1 or -1)
        int absolute_winner = simulate(sim_board);

        // 4. Backpropagation
        while (node != nullptr) {
            node->visits++;
            if (node->parent != nullptr) {
                // The player who made the decision to arrive at THIS node was the parent
                int parent_player = node->parent->player_to_move;
                if (absolute_winner == parent_player) {
                    node->score += 1.0;
                } else if (absolute_winner == 0) {
                    node->score += 0.5; // Draw
                }
            }
            node = node->parent;
        }
        nodes_searched++;
    }

    // Wrap up results
    SearchResult result;
    result.nodes = nodes_searched;

    if (!root.children.empty()) {
        MCTSNode* best_child = nullptr;
        int max_visits = -1;

        // Choose the root child with the highest visit count (standard for robust MCTS)
        for (const auto& child : root.children) {
            if (child->visits > max_visits) {
                max_visits = child->visits;
                best_child = child.get();
            }
        }

        if (best_child) {
            result.best_move = std::make_unique<Moves::Move>(*(best_child->move));

            // Map the win rate [0.0 to 1.0] back to an Alpha-Beta style score [-MATE to MATE]
            double win_rate = best_child->score / best_child->visits;
            result.score = static_cast<int>((win_rate * 2.0 - 1.0) * MATE);
        }
    }

    return result;
}

} // namespace Santorini