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

inline void score_moves(std::vector<std::unique_ptr<Moves::Move>>& moves, const Board& board) {
    for (auto& mv : moves) {
        int from_h = board.get_blocks()[mv->from_sq];
        int to_h = board.get_blocks()[mv->to_sq()];
        mv->score = (to_h - from_h) * 10 + (Constants::DOUBLE_NEIGHBORS[mv->to_sq()] - Constants::DOUBLE_NEIGHBORS[mv->from_sq]);
    }
}

inline void pick_move(std::vector<std::unique_ptr<Moves::Move>>& moves, size_t start_index) {
    size_t best_idx = start_index;
    int best_score = moves[best_idx]->score;
    for (size_t i = start_index + 1; i < moves.size(); ++i) {
        if (moves[i]->score > best_score) {
            best_idx = i;
            best_score = moves[i]->score;
        }
    }
    if (best_idx != start_index) {
        std::swap(moves[start_index], moves[best_idx]);
    }
}

int search(SearchInfo& search_info, int depth, int ply, int alpha, int beta, TranspositionTable& tt, bool allow_null = true);

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
        if (worker_move_searched[move->from_sq]) {
            continue;
        }

        int from_h = search_info.board.get_blocks()[move->from_sq];
        int to_h = search_info.board.get_blocks()[move->to_sq()];

        int god_index = (search_info.board.get_turn() == 1) ? 0 : 1;
        Constants::God god = search_info.board.get_gods()[god_index];

        bool is_climb = to_h > from_h;
        bool is_pan_drop = (god == Constants::God::PAN && from_h - to_h >= 2);

        // Only search "non-quiet" moves like climbs or special god moves.
        if (!(is_climb || is_pan_drop)) {
            continue;
        }

        search_info.board.make_move(*move);
        // Mark this worker's starting square as searched for this node.
        worker_move_searched[move->from_sq] = true;
        int score = -qsearch(search_info, -beta, -alpha);
        search_info.board.unmake_move(*move);

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

inline int search(SearchInfo& search_info, int depth, int ply, int alpha, int beta, TranspositionTable& tt, bool allow_null) {
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

    if (allow_null && depth >= adaptive_null_reduction(ply) + 1) {
        bool prevent_up = search_info.board.get_prevent_up_next_turn();
        search_info.board.make_null_move();
        int score = -search(search_info, depth - 1 - adaptive_null_reduction(ply),
                        ply + 1, -beta, -beta + 1, tt, false);
        search_info.board.unmake_null_move(prevent_up);
        if (score >= beta) {
            return beta;
        }
    }

    auto [tt_move_ptr, tt_score_opt] = tt.probe(search_info.board, alpha, beta, depth);
    if (tt_move_ptr != nullptr) {
        return *tt_score_opt;
    }

    auto moves = search_info.board.generate_moves();
    if (moves.empty()) {
        return -MATE + ply;
    }

    int max_score = -MATE * 100;
    std::unique_ptr<Moves::Move> best_move = nullptr;
    int original_alpha = alpha;

    score_moves(moves, search_info.board);

    for (size_t i = 0; i < moves.size(); ++i) {
        pick_move(moves, i);
        auto& move = moves[i];

        search_info.board.make_move(*move);
        int curr_score = -search(search_info, depth - 1, ply + 1, -beta, -alpha, tt);
        search_info.board.unmake_move(*move);

        if (search_info.quit) {
            search_info.bestMove = nullptr;
            return 0;
        }

        if (curr_score > max_score) {
            max_score = curr_score;
            best_move = clone_move(*move);
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

    search_info.bestMove = std::move(best_move);
    if (!(ply == 0 && search_info.quit) && search_info.bestMove) {
        if (alpha != original_alpha) {
            tt.store(search_info.board, *search_info.bestMove, max_score, depth, 'E');
        } else {
            tt.store(search_info.board, *search_info.bestMove, alpha, depth, 'A');
        }
    }

    return alpha;
}

inline std::unique_ptr<Moves::Move> get_best_move(
    Board& board,
    int remaining_time_ms,
    TranspositionTable& tt,
    std::optional<int> max_depth = std::nullopt)
{
    auto thinking_time = std::chrono::milliseconds(remaining_time_ms / 10);
    auto end_time = std::chrono::high_resolution_clock::now() + thinking_time;

    std::unique_ptr<Moves::Move> best_move = nullptr;
    int prev_score = 0;

    for (int depth = 1; ; ++depth) {
        int alpha = std::max(-MATE, prev_score - ASP_WINDOW);
        int beta = std::min(MATE, prev_score + ASP_WINDOW);

        while (true) {
            SearchInfo si(board, depth, end_time);
            int score = search(si, depth, 0, alpha, beta, tt);

            if (si.quit) {
                return best_move;
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

        auto [pv_move_ptr, pv_score_opt] = tt.probe_pv_move(board);
        if (pv_move_ptr != nullptr) {
            best_move = clone_move(*pv_move_ptr);
            if(pv_score_opt.has_value()) prev_score = *pv_score_opt;
        }

        if (is_mate(prev_score) || (max_depth.has_value() && depth >= *max_depth)) {
            return best_move;
        }

        if (depth >= 100) {
            return best_move;
        }
    }
}

} // namespace Santorini
