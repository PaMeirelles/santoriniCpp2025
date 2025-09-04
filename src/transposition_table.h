#pragma once

#include <vector>
#include <optional>
#include <memory>
#include "board.h"
#include "moves.h"

namespace Santorini {

struct TTEntry {
    uint64_t hash_key;
    std::optional<Moves::Move> move; // Store move by value for efficiency
    int depth;
    int score;
    char flag; // 'A': upper, 'B': lower, 'E': exact

    // Constructor
    TTEntry(uint64_t key, const Moves::Move& m, int d, int s, char f)
        : hash_key(key), move(m), depth(d), score(s), flag(f) {}
};

class TranspositionTable {
public:
    size_t num_entries;
    std::vector<std::optional<TTEntry>> table;
    int new_writes = 0;
    int overwrites = 0;
    int hits = 0;
    int cuts = 0;

    explicit TranspositionTable(size_t num_entries = 1 << 22)
        : num_entries(num_entries) {
        table.resize(num_entries);
    }

    void clear() {
        for (auto& entry : table) {
            entry.reset();
        }
        new_writes = 0;
        overwrites = 0;
        hits = 0;
        cuts = 0;
    }

    void store(const Board& board, const Moves::Move& move, int score, int depth, char flag) {
        uint64_t key = board.get_hash();
        size_t idx = key % num_entries;
        if (!table[idx].has_value()) {
            new_writes++;
        } else {
            overwrites++;
        }
        table[idx].emplace(key, move, depth, score, flag);
    }

    std::pair<const Moves::Move*, std::optional<int>> probe(const Board& board, int alpha, int beta, int depth) {
        uint64_t key = board.get_hash();
        size_t index = key % num_entries;
        const auto& entry_opt = table[index];

        if (entry_opt && entry_opt->hash_key == key) {
            hits++;
            const TTEntry& entry = *entry_opt;
            const Moves::Move* move_ptr = entry.move ? &(*entry.move) : nullptr;

            // If the stored search depth is sufficient, we can potentially use the score for a cutoff.
            if (entry.depth >= depth) {
                if (entry.flag == 'A' && entry.score <= alpha) {
                    cuts++;
                    // The stored score is an upper bound and is already too low.
                    return {move_ptr, alpha};
                }
                if (entry.flag == 'B' && entry.score >= beta) {
                    cuts++;
                    // The stored score is a lower bound and is already high enough for a cutoff.
                    return {move_ptr, beta};
                }
                if (entry.flag == 'E') {
                    // We found an exact score from a search of the same or greater depth.
                    return {move_ptr, entry.score};
                }
            }

            // If we couldn't cause a cutoff but found a matching entry,
            // return the move anyway to help with move ordering.
            return {move_ptr, std::nullopt};
        }

        // No matching entry was found.
        return {nullptr, std::nullopt};
    }

    std::pair<const Moves::Move*, std::optional<int>> probe_pv_move(const Board& board) {
        uint64_t key = board.get_hash();
        size_t index = key % num_entries;
        const auto& entry_opt = table[index];

        if (entry_opt && entry_opt->hash_key == key) {
            const Moves::Move* move_ptr = entry_opt->move ? &(*entry_opt->move) : nullptr;
            return {move_ptr, entry_opt->score};
        }
        return {nullptr, std::nullopt};
    }

    std::vector<Moves::Move> probe_pv_line(Board board) {
        std::vector<Moves::Move> pv_line;
        for (int i = 0; i < 10; ++i) { // Limit PV line length to avoid infinite loops
            uint64_t key = board.get_hash();
            size_t index = key % num_entries;
            const auto& entry_opt = table[index];

            if (!entry_opt || entry_opt->hash_key != key || !entry_opt->move) {
                break;
            }

            pv_line.push_back(*entry_opt->move);
            board.make_move(*entry_opt->move);
        }
        return pv_line;
    }
};

} // namespace Santorini
