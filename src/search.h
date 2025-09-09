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
            {8, -2, 0, 0}
        },
        {
            {2, 16, -8, 0}
        },
        {
            {0, 16, 32, -32}
        },
        {
            {0, 16, -4, -2}
        }
    }};

constexpr std::array<std::array<int, 4>, 4> BLOCK_SCORING_DOUBLE =
    {{
        {
            {0, 0, 0, 0}
        },
        {
            {1,-1, -2, 0}
        },
        {
            {1, 2, -1, 0}
        },
        {
            {1, 0, -2, 0}
        }
    }};
inline void score_moves(std::vector<Moves::Move> &moves, const Board& board, const KillerMoves& k_moves, const int ply) {
    auto k1 = k_moves.killers[ply][0];
    auto k2 = k_moves.killers[ply][1];
    auto k3 = k_moves.killers[ply][2];

    for (auto& mv : moves) {
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
        int mover = board.get_workers_map()[mv.from_sq];
        switch (mover) {
            case 0: ally = 1; enemy_1 = 2; enemy_2 = 3; break;
            case 1: ally = 0; enemy_1 = 2; enemy_2 = 3; break;
            case 2: ally = 3; enemy_1 = 0; enemy_2 = 1; break;
            case 3: ally = 2; enemy_1 = 0; enemy_2 = 1; break;
        }

        int current_block_score = 0;
        int worker_height = board.get_blocks()[mv.to_sq];

        // 1. Score the primary build
        int build_loc_height = board.get_blocks()[mv.build_sq];
        current_block_score += BLOCK_SCORING_SINGLE[worker_height][build_loc_height];
        if (Moves::is_adjacent(board.get_workers()[ally], mv.build_sq)) {
            current_block_score += BLOCK_SCORING_SINGLE[board.get_blocks()[board.get_workers()[ally]]][build_loc_height];
        }
        if (Moves::is_adjacent(board.get_workers()[enemy_1], mv.build_sq)) {
            current_block_score -= BLOCK_SCORING_SINGLE[board.get_blocks()[board.get_workers()[enemy_1]]][build_loc_height];
        }
        if (Moves::is_adjacent(board.get_workers()[enemy_2], mv.build_sq)) {
            current_block_score -= BLOCK_SCORING_SINGLE[board.get_blocks()[board.get_workers()[enemy_2]]][build_loc_height];
        }

        // 2. Score the extra build, if it exists
        if (mv.extra_build_sq) {
            const auto& matrix = (mv.extra_build_sq.value() == mv.build_sq)
                ? BLOCK_SCORING_DOUBLE
                : BLOCK_SCORING_SINGLE;
            int extra_build_loc_height = board.get_blocks()[mv.extra_build_sq.value()];

            current_block_score += matrix[worker_height][extra_build_loc_height];
            if (Moves::is_adjacent(board.get_workers()[ally], mv.extra_build_sq.value())) {
                current_block_score += BLOCK_SCORING_SINGLE[board.get_blocks()[board.get_workers()[ally]]][extra_build_loc_height];
            }
            if (Moves::is_adjacent(board.get_workers()[enemy_1], mv.extra_build_sq.value())) {
                current_block_score -= BLOCK_SCORING_SINGLE[board.get_blocks()[board.get_workers()[enemy_1]]][extra_build_loc_height];
            }
            if (Moves::is_adjacent(board.get_workers()[enemy_2], mv.extra_build_sq.value())) {
                current_block_score -= BLOCK_SCORING_SINGLE[board.get_blocks()[board.get_workers()[enemy_2]]][extra_build_loc_height];
            }
        }

        // --- Final Score Calculation ---
        int from_h = board.get_blocks()[mv.from_sq];
        int to_h = board.get_blocks()[mv.to_sq];
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

inline int qsearch(SearchInfo& search_info, int alpha, int beta) {
    search_info.nodes++;

    if ((search_info.nodes % CHECK_EVERY) == 0 && std::chrono::high_resolution_clock::now() > search_info.end_time) {
        search_info.quit = true;
        return 0;
    }

    int state = search_info.board.check_state();
    if (state != 0) {
        return (state == search_info.board.get_turn()) ? MATE - 1 : -MATE + 1;
    }

    int stand_pat = evaluate(search_info.board);
    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // Use a simple boolean array for tracking visited squares.
    // It's much faster than std::set for this purpose.
    bool worker_move_searched[25] = {false};
    auto moves = search_info.board.generate_moves();

    for (auto& move : moves) {
        // If we've already searched a move for the worker on this starting square, skip.
        if (worker_move_searched[move.from_sq]) {
            continue;
        }

        int from_h = search_info.board.get_blocks()[move.from_sq];
        int to_h = search_info.board.get_blocks()[move.to_sq];

        int god_index = (search_info.board.get_turn() == 1) ? 0 : 1;
        Constants::God god = search_info.board.get_gods()[god_index];

        bool is_climb = to_h > from_h;
        bool is_pan_drop = (god == Constants::God::PAN && from_h - to_h >= 2);

        // Only search "non-quiet" moves like climbs or special god moves.
        if (!(is_climb || is_pan_drop)) {
            continue;
        }

        search_info.board.make_move(move);
        // Mark this worker's starting square as searched for this node.
        worker_move_searched[move.from_sq] = true;
        int score = -qsearch(search_info, -beta, -alpha);

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

    int state = search_info.board.check_state();
    if (state != 0) {

        return (state == search_info.board.get_turn()) ? MATE - ply : -MATE + ply;
    }

    if (depth <= 0) {

        return qsearch(search_info, alpha, beta);
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
    std::optional<int> tt_score_opt; // Or whatever your score type is
    bool tt_hit = tt.probe(search_info.board, alpha, beta, depth, &tt_move_opt, &tt_score_opt);
    // If the probe returns a score, it's a valid cutoff.
    if (tt_hit) {
        return *tt_score_opt;
    }

    int max_score = -MATE * 100;
    std::unique_ptr<Moves::Move> best_move = nullptr;
    int original_alpha = alpha;

    // --- Phase 0: Search TT move first if it exists ---
    if (tt_move_opt.has_value()) {
        const Moves::Move& move = *tt_move_opt;
        if (search_info.board.is_valid_move(move)) {
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

                        return beta; // Beta cutoff from TT move
                    }
                    alpha = max_score;
                }
            }
        }
    }


    // --- Phase 1: Generate and search climber moves first ---
    auto climber_moves = search_info.board.generate_climber_moves();
    score_moves(climber_moves, search_info.board, k_moves, ply);

    for (size_t i = 0; i < climber_moves.size(); ++i) {
        pick_move(climber_moves, i);
        auto& move = climber_moves[i];

        // Skip if this is the TT move we already searched
        if (tt_move_opt.has_value() && move == *tt_move_opt) {
            continue;
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
                    // Beta cutoff from a climber move
                    search_info.bestMove = std::move(best_move);
                    tt.store(search_info.board, *search_info.bestMove, beta, depth, 'B');

                    return beta;
                }
                alpha = max_score;
            }
        }
    }

    // --- Phase 2: Generate and search quiet moves ---
    auto quiet_moves = search_info.board.generate_quiet_moves();

    // Check for loss (no moves available). This is adjusted to not fire if we found a TT move.
    if (best_move == nullptr && climber_moves.empty() && quiet_moves.empty()) {

        return -MATE + ply;
    }

    score_moves(quiet_moves, search_info.board, k_moves, ply);

    for (size_t i = 0; i < quiet_moves.size(); ++i) {
        pick_move(quiet_moves, i);
        auto& move = quiet_moves[i];

        // Skip if this is the TT move we already searched
        if (tt_move_opt.has_value() && move == *tt_move_opt) {
            continue;
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
                    // Beta cutoff from a quiet move
                    k_moves.add(ply, move);
                    search_info.bestMove = std::move(best_move);
                    tt.store(search_info.board, *search_info.bestMove, beta, depth, 'B');

                    return beta;
                }
                alpha = max_score;
            }
        }
    }

    // Store result in Transposition Table
    search_info.bestMove = std::move(best_move);
    if (!(ply == 0 && search_info.quit) && search_info.bestMove) {
        if (alpha != original_alpha) {
            // Found a better move that raised alpha
            tt.store(search_info.board, *search_info.bestMove, max_score, depth, 'E');
        } else {
            // Failed to raise alpha, all moves were worse
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