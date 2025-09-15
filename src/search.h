#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <set>

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

inline int adaptive_null_reduction(int depth) {
    if (depth >= 8) return 3;
    if (depth >= 4) return 2;
    return 1;
}

inline bool is_mate(int score) {
    return score > (MATE - 100) || score < (-MATE + 100);
}

struct SearchResult {
    std::unique_ptr<Moves::Move> best_move = nullptr;
    int score = 0;
    long nodes = 0;
};

struct SearchInfo {
    Board& board;
    int depth;
    std::chrono::time_point<std::chrono::high_resolution_clock> end_time;
    long nodes = 0;
    bool quit = false;
    std::unique_ptr<Moves::Move> bestMove = nullptr;

    SearchInfo(Board& b, int d, std::chrono::time_point<std::chrono::high_resolution_clock> et)
        : board(b), depth(d), end_time(et) {}
};

inline int evaluate(const Board& board) {
    return score_position(board) * board.get_turn();
}

constexpr std::array<std::array<int, 5>, 4> HEIGHT_SCORING =
    {{
        {
            {0, 1, 3, 0, 0}
        },
        {
            {-1, 0, 2, 4, 0}
        },
        {
            {-2, -1, 0, 4, 0}
        },
        {
            {-1, 0, 2, 0, 0}
        }
    }};

constexpr std::array<std::array<int, 5>, 4> BLOCK_SCORING_SINGLE =
    {{
        {
            {8, -8, 0, 0, 0}
        },
        {
            {2, 16, -16, 0, 0}
        },
        {
            {0, 4, 32, -32, 0}
        },
        {
            {0, 16, -16, -2, 0}
        }
    }};

constexpr std::array<std::array<int, 5>, 4> BLOCK_SCORING_DOUBLE =
    {{
        {
            {0, 0, 0, 0, 0}
        },
        {
            {16, -2, -16, 0, 0}
        },
        {
            {2, 32, -2, 0, 0}
        },
        {
            {2, 0, -4, 0, 0}
        }
    }};

constexpr std::array<std::array<int, 5>, 4> BLOCK_SCORING_DOME =
    {{
        {
            {0, 0, 0, 0, 0}
        },
        {
            {0, -2, -16, 0, 0}
        },
        {
            {0, 0, -2, -32, 0}
        },
        {
            {0, 0, -4, 0, 0}
        }
    }};


// --- Helper Struct for Move Scoring Cache (Moved Outside) ---
struct InfluenceCache {
    static constexpr int CACHE_SENTINEL = 30000;

    std::array<int, 25> single_build;
    std::array<int, 25> double_build;
    std::array<int, 25> dome_build;

    InfluenceCache() {
        single_build.fill(CACHE_SENTINEL);
        double_build.fill(CACHE_SENTINEL);
        dome_build.fill(CACHE_SENTINEL);
    }
};

inline void score_moves(std::vector<Moves::Move>& moves, const Board& board, const KillerMoves& k_moves, const int ply) {
    if (moves.empty()) {
        return;
    }

    auto k1 = k_moves.killers[ply][0];
    auto k2 = k_moves.killers[ply][1];
    auto k3 = k_moves.killers[ply][2];
    auto current_god = board.get_current_god();

    // The cache is now an instance of the struct defined outside the function.
    std::array<InfluenceCache, 4> influence_cache;

    // Helper lambda to get influence: checks cache, computes if necessary, then returns value.
    auto get_influence = [&](int mover, sq_i build_sq, const auto& matrix, std::array<int, 25>& cache) -> int {
        if (cache[build_sq] != InfluenceCache::CACHE_SENTINEL) {
            return cache[build_sq]; // Cache Hit
        }

        // Cache Miss: Compute, store, and return
        sq_i ally, enemy_1, enemy_2;
        switch (mover) {
            case 0: ally = 1; enemy_1 = 2; enemy_2 = 3; break;
            case 1: ally = 0; enemy_1 = 2; enemy_2 = 3; break;
            case 2: ally = 3; enemy_1 = 0; enemy_2 = 1; break;
            case 3: ally = 2; enemy_1 = 0; enemy_2 = 1; break;
            default: throw std::runtime_error("Invalid mover index");
        }

        const auto& workers = board.get_workers();
        const auto& blocks = board.get_blocks();
        int influence = 0;
        sq_i build_h = blocks[build_sq];

        // Ally contribution
        if (Moves::is_adjacent(workers[ally], build_sq)) {
            influence += matrix[blocks[workers[ally]]][build_h];
        }
        // Enemy contributions
        if (Moves::is_adjacent(workers[enemy_1], build_sq)) {
            influence -= matrix[blocks[workers[enemy_1]]][build_h];
        }
        if (Moves::is_adjacent(workers[enemy_2], build_sq)) {
            influence -= matrix[blocks[workers[enemy_2]]][build_h];
        }

        cache[build_sq] = influence; // Store result in cache
        return influence;
    };

    for (auto& mv : moves) {
        // --- High-Priority Scoring (Pan Win & Killer Moves) ---
        if (current_god == Constants::God::PAN && board.get_blocks()[mv.from_sq] >= 2 && board.get_blocks()[mv.to_sq] == 0) {
            mv.score = 1000000; continue;
        }
        if (k1.has_value() && *k1 == mv) { mv.score = 900000; continue; }
        if (k2.has_value() && *k2 == mv) { mv.score = 800000; continue; }
        if (k3.has_value() && *k3 == mv) { mv.score = 700000; continue; }

        // --- Block Scoring using On-Demand Caching ---
        int mover = board.get_workers_map()[mv.from_sq];
        int current_block_score = 0;
        const sq_i worker_h = board.get_blocks()[mv.to_sq];

        if (mv.extra_build_sq.has_value() && mv.extra_build_sq.value() == mv.build_sq) {
            // Hephaestus-style double build
            int influence = get_influence(mover, mv.build_sq, BLOCK_SCORING_DOUBLE, influence_cache[mover].double_build);
            current_block_score = BLOCK_SCORING_DOUBLE[worker_h][board.get_blocks()[mv.build_sq]] + influence;
        } else {
            // Standard build or Atlas dome
            if (mv.dome) {
                int influence = get_influence(mover, mv.build_sq, BLOCK_SCORING_DOME, influence_cache[mover].dome_build);
                current_block_score = BLOCK_SCORING_DOME[worker_h][board.get_blocks()[mv.build_sq]] + influence;
            } else {
                int influence = get_influence(mover, mv.build_sq, BLOCK_SCORING_SINGLE, influence_cache[mover].single_build);
                current_block_score = BLOCK_SCORING_SINGLE[worker_h][board.get_blocks()[mv.build_sq]] + influence;
            }
            // Demeter/Prometheus-style extra build
            if (mv.extra_build_sq.has_value()) {
                const sq_i extra_build_sq = mv.extra_build_sq.value();
                int influence = get_influence(mover, extra_build_sq, BLOCK_SCORING_SINGLE, influence_cache[mover].single_build);
                current_block_score += BLOCK_SCORING_SINGLE[worker_h][board.get_blocks()[extra_build_sq]] + influence;
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

inline int qsearch(SearchInfo& search_info, int alpha, int beta, KillerMoves& k_moves, int ply) {
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

    if (climber_moves.empty()) {
        return stand_pat;
    }

    for (const auto& move : climber_moves) {
        if (move.winning) {
            return MATE - ply;
        }
    }

    bool visited_to_sq[25] = {false};
    score_moves(climber_moves, search_info.board, k_moves, ply);

    for (size_t i = 0; i < climber_moves.size(); ++i) {
        pick_move(climber_moves, i);
        auto& move = climber_moves[i];
        if (visited_to_sq[move.to_sq]) {
            continue;
        }
        search_info.board.make_move(move);
        visited_to_sq[move.to_sq] = true;
        int score = -qsearch(search_info, -beta, -alpha, k_moves, ply+1);
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
    bool tt_hit = tt.probe(search_info.board, alpha, beta, depth, &tt_move_opt, &tt_score_opt);
    if (tt_hit) {
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

    auto climber_moves = search_info.board.generate_climber_moves();
    score_moves(climber_moves, search_info.board, k_moves, ply);

    for (size_t i = 0; i < climber_moves.size(); ++i) {
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

    auto quiet_moves = search_info.board.generate_quiet_moves();

    if (best_move == nullptr && climber_moves.empty() && quiet_moves.empty()) {
        return -MATE + ply;
    }

    score_moves(quiet_moves, search_info.board, k_moves, ply);
    std::sort(quiet_moves.begin(), quiet_moves.end());
    for (size_t i = 0; i < quiet_moves.size(); ++i) {
        auto& move = quiet_moves[i];

        if (tt_move_opt.has_value() && move == *tt_move_opt) {
            continue;
        }
        if (move.winning) {
            tt.store(search_info.board, move, MATE - ply, depth, 'E');
            k_moves.add(ply, move);
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
    long nodes_searched = 0; // To store nodes from last completed iteration
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
            return result;
        }
    }
}
} // namespace Santorini