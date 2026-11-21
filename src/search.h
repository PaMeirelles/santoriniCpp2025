#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <set>
#include <cmath>

#include "board.h"
#include "moves.h"
#include "transposition_table.h"
#include "evaluation.h"
#include "killer_moves.h"


namespace Santorini {

// Use the correct type alias as defined in your constants/moves headers
using sq_i = int8_t;

constexpr int MATE = 10000;
constexpr int CHECK_EVERY = 4096;
constexpr int ASP_WINDOW = 50;
constexpr int MIN_DEPTH = 2;

constexpr int RFP_DEPTH = 4;
constexpr int RFP_MARGIN = 250;

constexpr int N_PROTECTED_MOVES = 2;
constexpr float LMR_FACTOR = 4;

constexpr int FUTILITY_PRUNING_MAX_DEPTH = 6;
constexpr int FUTILITY_BASE_MARGIN = -50;
constexpr int FUTILITY_DEPTH_MARGIN = 250;

constexpr int RAZOR_DEPTH = 4;
constexpr int RAZOR_MARGIN = 150;


inline int get_lmr_reduction(const int depth, const int move_index) {
    if (move_index <= N_PROTECTED_MOVES) {
        return 0;
    }
    static const auto reductions = [] {
        std::array<std::array<double, 1024>, 101> values{};
        for (int d = 1; d <= 100; ++d)
            for (int m = 3; m < 1024; ++m)
                values[d][m] = (log2(d) * log2(m)) / LMR_FACTOR;
        return values;
    }();
    auto reduction = depth > 0 && depth <= 100 && move_index < 1024
        ? reductions[depth][move_index]
        : (log2(depth) * log2(move_index)) / LMR_FACTOR;

    if (depth - reduction < MIN_DEPTH) {
        reduction = depth - MIN_DEPTH;
    }

    if (reduction < 0) return 0;

    return static_cast<int>(reduction);
}


inline int adaptive_null_reduction(int depth) {
    if (depth >= 8) return 3;
    if (depth >= 4) return 2;
    return 1;
}



inline bool is_mate(int score) {
    return score > (MATE - 100) || score < (-MATE + 100);
}

struct SearchResult {
    int completed_depth = 0;
    bool timed_out = false;
    std::unique_ptr<Moves::Move> best_move = nullptr;
    int score = 0;
    std::int64_t nodes = 0;
};

struct SearchInfo {
    Board& board;
    int depth;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
    std::int64_t nodes = 0;
    bool quit = false;
    std::unique_ptr<Moves::Move> bestMove = nullptr;

    SearchInfo(Board& b, int d, std::chrono::time_point<std::chrono::high_resolution_clock> et)
        : board(b), depth(d), end_time(et) {}
};

inline int evaluate(const Board& board) {
    return score_position(board) * board.get_turn();
}

constexpr std::array<std::array<int, 4>, 4> HEIGHT_SCORING =
    {{
        {
            {0, 1, 3, 0}
        },
        {
            {-1, 0, 2, 4}
        },
        {
            {-2, -1, 0, 4}
        },
        {
            {-1, 0, 2, 0}
        }
    }};

constexpr std::array<std::array<int, 4>, 4> BLOCK_SCORING_SINGLE =
    {{
        {
            {8, -8, 0, 0}
        },
        {
            {2, 16, -16, 0}
        },
        {
            {0, 4, 32, -32}
        },
        {
            {0, 16, -16, -2}
        }
    }};

constexpr std::array<std::array<int, 4>, 4> BLOCK_SCORING_DOUBLE =
    {{
        {
            {0, 0, 0, 0}
        },
        {
            {16, -2, -16, 0}
        },
        {
            {2, 32, -2, 0}
        },
        {
            {2, 0, -4, 0}
        }
    }};

constexpr std::array<std::array<int, 4>, 4> BLOCK_SCORING_DOME =
    {{
        {
            {0, 0, 0, 0}
        },
        {
            {0, -2, -16, 0}
        },
        {
            {0, 0, -2, -32}
        },
        {
            {0, 0, -4, 0}
        }
    }};


inline void score_moves(std::vector<Moves::Move> &moves, const Board& board, const KillerMoves& k_moves, const int ply) {
    auto k1 = ply >= 0 && ply < MAX_PLY ? k_moves.killers[ply][0] : std::nullopt;
    auto k2 = ply >= 0 && ply < MAX_PLY ? k_moves.killers[ply][1] : std::nullopt;
    auto k3 = ply >= 0 && ply < MAX_PLY ? k_moves.killers[ply][2] : std::nullopt;
    auto current_god = board.get_current_god();
    for (auto& mv : moves) {
        // Pan's winning move should be scored highly, then we continue to the next move.
        if (current_god == Constants::God::PAN && board.get_blocks()[mv.from_sq] >= 2 && board.get_blocks()[mv.to_sq] == 0) {
            mv.score = 1000000;
            return;
        }
        // --- Killer Move Heuristic (unchanged) ---
        if (k1.has_value() && *k1==mv) {
            mv.score = 900000;
            continue;
        }
        if (k2.has_value() && *k2==mv) {
            mv.score = 800000;
            continue;
        }
        if (k3.has_value() && *k3==mv) {
            mv.score = 700000;
            continue;
        }

        sq_i ally, enemy_1, enemy_2;

        switch (board.get_workers_map()[mv.from_sq]) {
            case 0: ally = 1; enemy_1 = 2; enemy_2 = 3; break;
            case 1: ally = 0; enemy_1 = 2; enemy_2 = 3; break;
            case 2: ally = 3; enemy_1 = 0; enemy_2 = 1; break;
            case 3: ally = 2; enemy_1 = 0; enemy_2 = 1; break;
            default: throw std::invalid_argument("Invalid move");
        }

        // --- Block Scoring Logic (REVISED) ---
        int current_block_score = 0;
        sq_i worker_height = board.get_blocks()[mv.to_sq];

        // Helper lambda to score a single build action based on a given scoring matrix
        auto score_a_build = [&](sq_i build_sq, const auto& matrix) {
            int score = 0;
            sq_i build_loc_height = board.get_blocks()[build_sq];
            score += matrix[worker_height][build_loc_height];
            if (adj_ok(board.get_workers()[ally], build_sq)) {
                score += matrix[board.get_blocks()[board.get_workers()[ally]]][build_loc_height];
            }
            if (adj_ok(board.get_workers()[enemy_1], build_sq)) {
                score -= matrix[board.get_blocks()[board.get_workers()[enemy_1]]][build_loc_height];
            }
            if (adj_ok(board.get_workers()[enemy_2], build_sq)) {
                score -= matrix[board.get_blocks()[board.get_workers()[enemy_2]]][build_loc_height];
            }
            return score;
        };

        // If the extra build is on the same square, use the double scoring matrix for the whole action.
        if (mv.extra_build_sq.has_value() && mv.extra_build_sq.value() == mv.build_sq) {
            current_block_score = score_a_build(mv.build_sq, BLOCK_SCORING_DOUBLE);
        } else {
            if (mv.dome) {
                current_block_score = score_a_build(mv.build_sq, BLOCK_SCORING_DOME);
            }
            else {
                current_block_score = score_a_build(mv.build_sq, BLOCK_SCORING_SINGLE);
            }
            if (mv.extra_build_sq.has_value()) {
                current_block_score += score_a_build(mv.extra_build_sq.value(), BLOCK_SCORING_SINGLE);
            }
        }

        // --- Final Score Calculation ---
        const sq_i from_h = board.get_blocks()[mv.from_sq];
        const sq_i to_h = board.get_blocks()[mv.to_sq];
        mv.score = HEIGHT_SCORING[from_h][to_h] * 800 +
            current_block_score * 10 +
            (Constants::DOUBLE_NEIGHBORS[mv.to_sq] - Constants::DOUBLE_NEIGHBORS[mv.from_sq]);
    }
}

inline void pick_move(std::vector<Moves::Move>& moves, size_t start_index) {
    size_t best_idx = start_index;
    int best_score = moves[best_idx].score;
    for (size_t i = start_index + 1; i < moves.size(); ++i) {
        if (moves[i].score > best_score) {
            best_idx = i;
            best_score = moves[i].score;
        }
    }
    if (best_idx != start_index) {
        std::swap(moves[start_index], moves[best_idx]);
    }
}

int search(SearchInfo& search_info, int depth, int ply, int alpha, int beta, TranspositionTable& tt,
    KillerMoves& k_moves, bool allow_null = true);

inline bool qsearch_reaches_new_height(const Board& board, const Moves::Move& move, int highest) {
    return board.get_blocks()[move.to_sq] > highest;
}

// Only the new quiet-defense extension is tied to this worker. Normal Horizon
// 12.3 climbs remain unchanged if its partner takes over the climbing line.
inline bool qsearch_defender_turn(const Board& board, int aggressor, bool active) {
    return active && aggressor >= 0 && ((aggressor < 2) != (board.get_turn() == 1));
}

inline bool qsearch_has_block_target(const Board& board, int aggressor) {
    // Displacement can change which local targets matter; defer to the move test.
    auto god = board.get_current_god();
    if (god == Constants::God::APOLLO || god == Constants::God::MINOTAUR) return true;
    const auto square = board.get_workers()[aggressor];
    const int height = board.get_blocks()[square];
    for (auto target : Constants::NEIGHBOURS[square]) {
        if (board.get_blocks()[target] == height + 1) return true;
    }
    return false;
}

// Legal quiet moves only. Project the worker displacement and at most two
// builds without make/unmake, evaluation, or another tactical search.
inline bool qsearch_blocks_aggressor(const Board& board, const Moves::Move& move, int aggressor) {
    int square = board.get_workers()[aggressor];
    if (move.to_sq == square) {
        if (move.god == Constants::God::APOLLO) square = move.from_sq;
        else if (move.god == Constants::God::MINOTAUR) square = 2 * move.to_sq - move.from_sq;
    }
    const int height = board.get_blocks()[square];
    auto blocks = [&](int target, int increments, bool dome) {
        const int before = board.get_blocks()[target];
        const int after = dome ? 4 : before + increments;
        return adj_ok(square, target) && before == height + 1 && before < 4 &&
            (after > height + 1 || after == 4);
    };
    const bool extra = move.extra_build_sq.has_value();
    const bool same = extra && *move.extra_build_sq == move.build_sq;
    return blocks(move.build_sq, same ? 2 : 1, move.dome) ||
        (extra && !same && blocks(*move.extra_build_sq, 1, false));
}

inline int qsearch(SearchInfo& search_info, int alpha, int beta, KillerMoves& k_moves, int ply,
    int highest_q_height = -1, int aggressor = -1, bool defense_active = true) {
    search_info.nodes++;

    if ((search_info.nodes % CHECK_EVERY) == 0 && std::chrono::high_resolution_clock::now() > search_info.end_time) {
        search_info.quit = true;
        return 0;
    }
    int stand_pat = evaluate(search_info.board);
    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    auto climber_moves = search_info.board.generate_climber_moves();

    for (const auto& move : climber_moves) {
        if (move.winning) {
            return MATE - ply;
        }
    }

    if (qsearch_defender_turn(search_info.board, aggressor, defense_active) &&
        qsearch_has_block_target(search_info.board, aggressor)) {
        for (const auto& move : search_info.board.generate_quiet_moves()) {
            if (qsearch_blocks_aggressor(search_info.board, move, aggressor))
                climber_moves.push_back(move);
        }
    }
    if (climber_moves.empty()) return stand_pat;

    bool visited_to_sq[25] = {false};
    score_moves(climber_moves, search_info.board, k_moves, ply);

    for (size_t i = 0; i < climber_moves.size(); ++i) {
        pick_move(climber_moves, i);
        auto& move = climber_moves[i];
        if (visited_to_sq[move.to_sq]) {
            continue;
        }
        const bool climbing = search_info.board.get_blocks()[move.to_sq] >
            search_info.board.get_blocks()[move.from_sq];
        if (climbing && !qsearch_reaches_new_height(search_info.board, move, highest_q_height)) {
            continue;
        }
        const int landing_height = climbing ? search_info.board.get_blocks()[move.to_sq] : highest_q_height;
        const int worker = search_info.board.get_workers_map()[move.from_sq];
        const int next_aggressor = aggressor < 0 ? worker : aggressor;
        const bool next_defense_active = defense_active &&
            !((worker < 2) == (next_aggressor < 2) && worker != next_aggressor);
        search_info.board.make_move(move);
        visited_to_sq[move.to_sq] = true;
        int score = -qsearch(search_info, -beta, -alpha, k_moves, ply+1,
            landing_height, next_aggressor, next_defense_active);
        search_info.board.unmake_move(move);

        if (search_info.quit) return 0;

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

inline int search(SearchInfo& search_info, int depth, int ply, int alpha, int beta,
    TranspositionTable& tt, KillerMoves& k_moves, bool allow_null) {
    search_info.nodes++;
    if ((search_info.nodes % CHECK_EVERY) == 0 && std::chrono::high_resolution_clock::now() > search_info.end_time) {
        search_info.quit = true;
        search_info.bestMove = nullptr;
        return 0;
    }
    if (depth <= 0) {
        return qsearch(search_info, alpha, beta, k_moves, ply);
    }
    // reverse futility pruning
    auto eval = evaluate(search_info.board);
    if (depth <= RFP_DEPTH && eval >= beta + RFP_MARGIN * depth) {
        return eval;
    }

    if (depth <= RAZOR_DEPTH && eval + RAZOR_MARGIN * depth < alpha) {
        auto qeval = qsearch(search_info, alpha, beta, k_moves, ply);

        if (qeval < alpha) {
            return qeval;
        }
    }

    // Null Move Pruning
    if (allow_null && depth >= adaptive_null_reduction(ply) + 1) {
        bool prevent_up = search_info.board.get_prevent_up_next_turn();
        search_info.board.make_null_move();
        int score = -search(search_info, depth - 1 - adaptive_null_reduction(ply),
                        ply + 1, -beta, -beta + 1, tt, k_moves, false);
        search_info.board.unmake_null_move(prevent_up);
        if (score >= beta) {
            return beta;
        }
    }
    std::optional<Moves::Move> tt_move_opt;
    std::optional<int> tt_score_opt;
    if (tt.probe(search_info.board, alpha, beta, depth, &tt_move_opt, &tt_score_opt)) {
        return *tt_score_opt;
    }

    int max_score = -MATE * 100;
    std::unique_ptr<Moves::Move> best_move = nullptr;
    int original_alpha = alpha;

    if (tt_move_opt.has_value()) {
        const Moves::Move& move = *tt_move_opt;
        if (search_info.board.is_valid_move(move)) {
            if (move.winning) {
                tt.store(search_info.board, move, MATE - ply, depth, 'E');
                search_info.bestMove = std::make_unique<Moves::Move>(move);
                return MATE - ply;
            }

            search_info.board.make_move(move);
            int curr_score = -search(search_info, depth - 1, ply + 1, -beta, -alpha, tt, k_moves);
            search_info.board.unmake_move(move);

            if (search_info.quit) {
                search_info.bestMove = nullptr;
                return 0;
            }

            if (curr_score > max_score) {
                max_score = curr_score;
                best_move = std::make_unique<Moves::Move>(move);
                if (max_score > alpha) {
                    if (max_score >= beta) {
                        search_info.bestMove = std::move(best_move);
                        tt.store(search_info.board, *search_info.bestMove, beta, depth, 'B');
                        return beta;
                    }
                    alpha = max_score;
                }
            }
        }
    }
    int move_count = 0;
    auto climber_moves = search_info.board.generate_climber_moves();
    score_moves(climber_moves, search_info.board, k_moves, ply);

    for (size_t i = 0; i < climber_moves.size(); ++i) {
        move_count++;
        pick_move(climber_moves, i);
        auto& move = climber_moves[i];

        if (tt_move_opt.has_value() && move == *tt_move_opt) {
            continue;
        }

        if (move.winning) {
            tt.store(search_info.board, move, MATE - ply, depth, 'E');
            search_info.bestMove = std::make_unique<Moves::Move>(move);
            return MATE - ply;
        }

        int curr_score;
        auto reduction = get_lmr_reduction(depth, move_count);
        search_info.board.make_move(move);
        curr_score = -search(search_info, depth - 1 - reduction, ply + 1, -beta, -alpha, tt, k_moves);

        if (reduction > 0 && curr_score > alpha) {
            curr_score = -search(search_info, depth - 1, ply + 1, -beta, -alpha, tt, k_moves);
        }
        search_info.board.unmake_move(move);

        if (search_info.quit) {
            search_info.bestMove = nullptr;
            return 0;
        }

        if (curr_score > max_score) {
            max_score = curr_score;
            best_move = std::make_unique<Moves::Move>(move);
            if (max_score > alpha) {
                if (max_score >= beta) {
                    search_info.bestMove = std::move(best_move);
                    tt.store(search_info.board, *search_info.bestMove, beta, depth, 'B');
                    return beta;
                }
                alpha = max_score;
            }
        }
    }

    auto quiet_moves = search_info.board.generate_quiet_moves();

    if (best_move == nullptr && climber_moves.empty() && quiet_moves.empty()) {
        return -MATE + ply;
    }



    score_moves(quiet_moves, search_info.board, k_moves, ply);
    std::sort(quiet_moves.begin(), quiet_moves.end());

    int static_eval = 0;
    const bool can_prune = depth < FUTILITY_PRUNING_MAX_DEPTH && !is_mate(alpha);
    if (can_prune) {
        static_eval = evaluate(search_info.board);
    }

    for (auto & move : quiet_moves) {
        if (can_prune) {
            const int margin = FUTILITY_BASE_MARGIN + (FUTILITY_DEPTH_MARGIN * depth * depth);
            if (static_eval + margin <= alpha) {
                break; // Exit the quiet move loop
            }
        }

        move_count++;
        if (tt_move_opt.has_value() && move == *tt_move_opt) {
            continue;
        }
        if (move.winning) {
            tt.store(search_info.board, move, MATE - ply, depth, 'E');
            k_moves.add(ply, move);
            search_info.bestMove = std::make_unique<Moves::Move>(move);
            return MATE - ply;
        }

        int curr_score;
        auto reduction = get_lmr_reduction(depth, move_count);
        search_info.board.make_move(move);
        curr_score = -search(search_info, depth - 1 - reduction, ply + 1, -beta, -alpha, tt, k_moves);

        if (reduction > 0 && curr_score > alpha) {
            curr_score = -search(search_info, depth - 1, ply + 1, -beta, -alpha, tt, k_moves);
        }
        search_info.board.unmake_move(move);

        if (search_info.quit) {
            search_info.bestMove = nullptr;
            return 0;
        }

        if (curr_score > max_score) {
            max_score = curr_score;
            best_move = std::make_unique<Moves::Move>(move);
            if (max_score > alpha) {
                if (max_score >= beta) {
                    k_moves.add(ply, move);
                    search_info.bestMove = std::move(best_move);
                    tt.store(search_info.board, *search_info.bestMove, beta, depth, 'B');
                    return beta;
                }
                alpha = max_score;
            }
        }
    }

    search_info.bestMove = std::move(best_move);
    if (!(ply == 0 && search_info.quit) && search_info.bestMove) {
        if (alpha != original_alpha) {
            tt.store(search_info.board, *search_info.bestMove, max_score, depth, 'E');
        } else {
            tt.store(search_info.board, *search_info.bestMove, max_score, depth, 'A');
        }
    }

    return max_score;
}

inline SearchResult get_best_move(
    Board& board,
    int remaining_time_ms,
    TranspositionTable& tt,
    std::optional<int> max_depth = std::nullopt)
{
    auto thinking_time = std::chrono::milliseconds(remaining_time_ms / 10);
    auto end_time = std::chrono::high_resolution_clock::now() + thinking_time;

    std::unique_ptr<Moves::Move> best_move = nullptr;
    int prev_score = 0;
    std::int64_t nodes_searched = 0; // To store nodes from last completed iteration
    KillerMoves killers;

    for (int depth = 1; ; ++depth) {
        killers.clear();
        int alpha = std::max(-MATE, prev_score - ASP_WINDOW);
        int beta = std::min(MATE, prev_score + ASP_WINDOW);

        SearchInfo si(board, depth, end_time); // Moved outside the while loop

        while (true) {
            int score = search(si, depth, 0, alpha, beta, tt, killers);

            if (si.quit) {
                SearchResult result;
                result.best_move = std::move(best_move);
                result.score = prev_score;
                result.nodes = nodes_searched;
                result.completed_depth = depth - 1;
                result.timed_out = true;
                return result;
            }

            if (score <= alpha) {
                if (alpha == -MATE) break;
                alpha = -MATE;
                continue;
            }
            if (score >= beta) {
                if (beta == MATE) break;
                beta = MATE;
                continue;
            }
            prev_score = score;
            break;
        }

        nodes_searched = si.nodes; // Update nodes count after a successful depth search

        auto [pv_move_ptr, pv_score_opt] = tt.probe_pv_move(board);
        if (pv_move_ptr != nullptr) {
            best_move =  std::make_unique<Moves::Move>(*pv_move_ptr);
            if(pv_score_opt.has_value()) prev_score = *pv_score_opt;
        }

        if (is_mate(prev_score) || (max_depth.has_value() && depth >= *max_depth) || depth >= 100) {
            SearchResult result;
            result.best_move = std::move(best_move);
            result.score = prev_score;
            result.nodes = nodes_searched;
            result.completed_depth = depth;
            return result;
        }
    }
}
} // namespace Santorini
