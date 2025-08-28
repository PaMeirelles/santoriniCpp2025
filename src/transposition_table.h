#pragma once

#include <vector>
#include <optional>
#include <memory>
#include "board.h"
#include "moves.h"

namespace Santorini {

// Forward declaration
inline std::unique_ptr<Moves::Move> clone_move(const Moves::Move& move);

struct TTEntry {
    uint64_t hash_key;
    std::unique_ptr<Moves::Move> move;
    int depth;
    int score;
    char flag; // 'A': upper, 'B': lower, 'E': exact

    // Constructor
    TTEntry(uint64_t key, const Moves::Move& m, int d, int s, char f);

    // --- Rule of Five: Explicitly define move/copy semantics ---
    // Delete copy constructor and copy assignment
    TTEntry(const TTEntry&) = delete;
    TTEntry& operator=(const TTEntry&) = delete;

    // Define move constructor
    TTEntry(TTEntry&& other) noexcept = default;

    // Define move assignment operator
    TTEntry& operator=(TTEntry&& other) noexcept = default;
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
        table.resize(num_entries); // Use resize instead of constructor with value
    }

    void clear() {
        // Replace table.assign with a loop to avoid copy operations.
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

        if (entry_opt && entry_opt->hash_key == key && entry_opt->depth >= depth) {
            hits++;
            const TTEntry& entry = *entry_opt;
            if (entry.flag == 'A' && entry.score <= alpha) {
                cuts++;
                return {entry.move.get(), alpha};
            }
            if (entry.flag == 'B' && entry.score >= beta) {
                cuts++;
                return {entry.move.get(), beta};
            }
            if (entry.flag == 'E') {
                return {entry.move.get(), entry.score};
            }
        }
        return {nullptr, std::nullopt};
    }

    std::pair<const Moves::Move*, std::optional<int>> probe_pv_move(const Board& board) {
        uint64_t key = board.get_hash();
        size_t index = key % num_entries;
        const auto& entry_opt = table[index];

        if (entry_opt && entry_opt->hash_key == key) {
            return {entry_opt->move.get(), entry_opt->score};
        }
        return {nullptr, std::nullopt};
    }

    std::vector<std::unique_ptr<Moves::Move>> probe_pv_line(Board board) {
        std::vector<std::unique_ptr<Moves::Move>> pv_line;
        for (int i = 0; i < 10; ++i) { // Limit PV line length to avoid infinite loops
            uint64_t key = board.get_hash();
            size_t index = key % num_entries;
            const auto& entry_opt = table[index];

            if (!entry_opt || entry_opt->hash_key != key || !entry_opt->move) {
                break;
            }

            pv_line.push_back(clone_move(*entry_opt->move));
            board.make_move(*entry_opt->move);
        }
        return pv_line;
    }
};

// Helper function to clone a move polymorphically
inline std::unique_ptr<Moves::Move> clone_move(const Moves::Move& move) {
    if (auto m = dynamic_cast<const Moves::ApolloMove*>(&move)) return std::make_unique<Moves::ApolloMove>(*m);
    if (auto m = dynamic_cast<const Moves::ArtemisMove*>(&move)) return std::make_unique<Moves::ArtemisMove>(*m);
    if (auto m = dynamic_cast<const Moves::AthenaMove*>(&move)) return std::make_unique<Moves::AthenaMove>(*m);
    if (auto m = dynamic_cast<const Moves::AtlasMove*>(&move)) return std::make_unique<Moves::AtlasMove>(*m);
    if (auto m = dynamic_cast<const Moves::DemeterMove*>(&move)) return std::make_unique<Moves::DemeterMove>(*m);
    if (auto m = dynamic_cast<const Moves::HephaestusMove*>(&move)) return std::make_unique<Moves::HephaestusMove>(*m);
    if (auto m = dynamic_cast<const Moves::HermesMove*>(&move)) return std::make_unique<Moves::HermesMove>(*m);
    if (auto m = dynamic_cast<const Moves::MinotaurMove*>(&move)) return std::make_unique<Moves::MinotaurMove>(*m);
    if (auto m = dynamic_cast<const Moves::PanMove*>(&move)) return std::make_unique<Moves::PanMove>(*m);
    if (auto m = dynamic_cast<const Moves::PrometheusMove*>(&move)) return std::make_unique<Moves::PrometheusMove>(*m);

    throw std::runtime_error("Unknown move type to clone");
}

inline TTEntry::TTEntry(uint64_t key, const Moves::Move& m, int d, int s, char f)
    : hash_key(key), depth(d), score(s), flag(f) {
    move = clone_move(m);
}

} // namespace Santorini
