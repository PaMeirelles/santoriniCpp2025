#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <random>

#include "board.h"
#include "evaluation.h"

namespace Santorini {

using sq_i = int8_t;
constexpr int MATE = 10000;

struct SearchResult {
    std::vector<int> best_action_sequence;
    int score = 0;
    long nodes = 0;
};

struct MCTSNode {
    int action_idx;
    MCTSNode* parent;
    std::vector<std::unique_ptr<MCTSNode>> children;

    int visits = 0;
    double total_value = 0.0;
    double prior_prob = 0.0;
    int player_to_move;
    bool is_expanded = false;

    MCTSNode(int action, MCTSNode* p, int ptm, double prob)
        : action_idx(action), parent(p), player_to_move(ptm), prior_prob(prob) {}

    double get_q() const {
        if (visits == 0) return 0.0;
        return total_value / visits;
    }

    double get_puct_score(int parent_ptm, double c_puct = 1.4) const {
        double q = get_q();
        if (player_to_move != parent_ptm) {
            q = -q;
        }
        int parent_visits = parent ? parent->visits : 1;
        double u = c_puct * prior_prob * std::sqrt(parent_visits) / (1.0 + visits);
        return q + u;
    }
};

inline SearchResult get_best_move(
    Board& board,
    int remaining_time_ms,
    std::optional<int> max_iterations_opt = std::nullopt)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::milliseconds(remaining_time_ms * 1 / 10);

    MCTSNode root(-1, nullptr, board.get_turn(), 1.0);
    long nodes_searched = 0;

    while (std::chrono::high_resolution_clock::now() < end_time) {
        if (max_iterations_opt && nodes_searched >= *max_iterations_opt) break;

        MCTSNode* node = &root;
        Board sim_board = board;

        // 1. SELECTION
        while (node->is_expanded && !node->children.empty()) {
            MCTSNode* best_child = nullptr;
            double best_score = -std::numeric_limits<double>::max();

            for (const auto& child : node->children) {
                double score = child->get_puct_score(node->player_to_move);
                if (score > best_score) {
                    best_score = score;
                    best_child = child.get();
                }
            }
            node = best_child;
            sim_board.make_action(node->action_idx);
        }

        // 2. EVALUATION & EXPANSION
        auto legal_actions = sim_board.generate_legal_actions();
        int terminal_state = 0;

        if (sim_board.get_won()) {
            terminal_state = sim_board.get_turn();
        } else if (legal_actions.empty()) {
            terminal_state = -sim_board.get_turn();
        }

        double v = 0.0;
        if (terminal_state != 0) {
            v = (terminal_state == sim_board.get_turn()) ? 1.0 : -1.0;
        } else {
            int current_phase = sim_board.get_current_phase();
            auto nn_out = evaluate_board_nn(sim_board, current_phase);
            v = nn_out.value;

            double total_p = 0.0;
            std::vector<double> probs;
            probs.reserve(legal_actions.size());

            for (int action_idx : legal_actions) {
                double p = nn_out.policy_probs[action_idx];
                probs.push_back(p);
                total_p += p;
            }

            // Pre-allocate to prevent vector resizing
            node->children.reserve(legal_actions.size());

            for (size_t i = 0; i < legal_actions.size(); ++i) {
                double normalized_p = (total_p > 0) ? (probs[i] / total_p) : (1.0 / legal_actions.size());
                node->children.push_back(std::make_unique<MCTSNode>(
                    legal_actions[i], node, sim_board.get_turn(), normalized_p
                ));
            }
            node->is_expanded = true;
        }

        // 3. BACKPROPAGATION
        int leaf_player = sim_board.get_turn();
        while (node != nullptr) {
            node->visits++;
            if (node->player_to_move == leaf_player) {
                node->total_value += v;
            } else {
                node->total_value -= v;
            }
            node = node->parent;
        }
        nodes_searched++;
    }

    SearchResult result;
    result.nodes = nodes_searched;

    if (!root.children.empty()) {
        MCTSNode* root_best = nullptr;
        int max_visits = -1;
        for (const auto& child : root.children) {
            if (child->visits > max_visits) {
                max_visits = child->visits;
                root_best = child.get();
            }
        }
        if (root_best) {
            double q = root_best->get_q();
            if (root_best->player_to_move != root.player_to_move) {
                q = -q;
            }
            result.score = static_cast<int>(q * MATE);
        }

        // EXTRACT FULL TURN SEQUENCE (Principal Variation)
        MCTSNode* curr = &root;
        Board temp_board = board;
        int initial_turn = temp_board.get_turn();

        while (temp_board.get_turn() == initial_turn) {
            MCTSNode* best_child = nullptr;

            if (curr && curr->is_expanded && !curr->children.empty()) {
                int max_child_visits = -1;
                for (const auto& child : curr->children) {
                    if (child->visits > max_child_visits) {
                        max_child_visits = child->visits;
                        best_child = child.get();
                    }
                }
            }

            if (best_child) {
                result.best_action_sequence.push_back(best_child->action_idx);
                temp_board.make_action(best_child->action_idx);
                curr = best_child;
            } else {
                auto legal_actions = temp_board.generate_legal_actions();
                if (legal_actions.empty()) break;

                int fallback_action = legal_actions[0];
                result.best_action_sequence.push_back(fallback_action);
                temp_board.make_action(fallback_action);
                curr = nullptr;
            }
        }
    }
    return result;
}

} // namespace Santorini