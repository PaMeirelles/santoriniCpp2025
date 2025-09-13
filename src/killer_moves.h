#pragma once

#include "moves.h"
#include <optional>

// Add near the top of the file, after the includes.
constexpr int MAX_PLY = 64; // A reasonable max search depth
namespace Santorini {
    class KillerMoves {
    public:
        // Store 2 killer moves per ply.
        std::array<std::array<std::optional<Moves::Move>, 3>, MAX_PLY> killers;

        KillerMoves() = default;

        void add(int ply, const Moves::Move& move) {
            if (ply >= MAX_PLY) return;
            // Avoid adding the same move twice.
            if (killers[ply][0].has_value() && *killers[ply][0] == move) return;

            // Shift the existing killer and add the new one.
            killers[ply][2] = killers[ply][1];
            killers[ply][1] = killers[ply][0];
            killers[ply][0] = move;
        }

        void clear() {
            for (auto& ply_killers : killers) {
                ply_killers[0].reset();
                // ply_killers[1].reset();
            }
        }
    };
}