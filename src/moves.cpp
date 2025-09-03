#include "moves.h"
#include "board.h" // It is now safe to include board.h here

#include <queue>

namespace Santorini::Moves {
    // --- Move::to_text Implementation ---
    std::string Move::to_text(const Santorini::Board& board) const {
        std::string text = square_to_text(from_sq);

        switch (god) {
            case Constants::God::ARTEMIS:
                // If not a simple adjacent move, find a PLAUSIBLE AND UNOCCUPIED intermediate square.
                if (!is_adjacent(from_sq, to_sq)) {
                    for (sq_i mid = 0; mid < 25; ++mid) {
                        if (mid != from_sq && mid != to_sq && is_adjacent(from_sq, mid)
                            && is_adjacent(to_sq, mid) && board.is_free(mid)) { // THE FIX: Check if the square is free
                            text += square_to_text(mid);
                            break; // Found a valid mid-point
                        }
                    }
                }
                text += square_to_text(to_sq);
                text += square_to_text(build_sq);
                break;

            case Constants::God::HERMES: {
                // Hermes pathfinding doesn't need board state as it assumes an empty grid path
                std::vector<sq_i> path = find_hermes_path(from_sq, to_sq);
                for (const auto step: path) {
                    text += square_to_text(step);
                }
                text += square_to_text(build_sq);
                break;
            }

            case Constants::God::DEMETER:
            case Constants::God::HEPHAESTUS:
            case Constants::God::PROMETHEUS:
                text += square_to_text(to_sq);
                text += square_to_text(build_sq);
                if (extra_build_sq) {
                    text += square_to_text(*extra_build_sq);
                }
                break;

            case Constants::God::ATLAS:
                text += square_to_text(to_sq);
                text += square_to_text(build_sq);
                if (dome) {
                    text += 'D';
                }
                break;

            default: // Handles Apollo, Minotaur, Pan, Athena, etc.
                text += square_to_text(to_sq);
                text += square_to_text(build_sq);
                break;
        }
        return text;
    }

} // namespace Santorini::Moves
