#include "moves.h"
#include "board.h" // It is now safe to include board.h here

#include <queue>

namespace Santorini::Moves {


    bool is_adjacent(const sq_i s1, const sq_i s2) {
        if (s1 == s2) return false;
        const auto r1 = s1 % 5;
        const auto c1 = s1 / 5;
        const auto r2 = s2 % 5;
        const auto c2 = s2 / 5;
        return abs(r1 - r2) <= 1 && abs(c1 - c2) <= 1;
    }

    /**
     * @brief Finds the shortest path for a Hermes move, considering the board state.
     *
     * Uses Breadth-First Search (BFS) to find a shortest sequence of squares on the
     * same height level, avoiding occupied squares.
     */
    std::vector<sq_i> find_hermes_path(sq_i from, sq_i to, const Santorini::Board& board) {
        if (from == to) {
            return {};
        }

        const int8_t start_height = board.get_blocks()[from];
        std::queue<std::vector<sq_i>> q;
        q.push({from});

        std::vector<bool> visited(25, false);
        visited[from] = true;

        while (!q.empty()) {
            std::vector<sq_i> path = q.front();
            q.pop();
            sq_i current = path.back();

            if (current == to) {
                path.erase(path.begin()); // Remove the 'from' square
                return path;
            }

            // Explore neighbors
            for (sq_i neighbor : Constants::NEIGHBOURS[current]) {
                // THE FIX: Check board state (free, same height)
                if (!visited[neighbor] && board.is_free(neighbor) && board.get_blocks()[neighbor] == start_height) {
                    visited[neighbor] = true;
                    std::vector<sq_i> new_path = path;
                    new_path.push_back(neighbor);
                    q.push(new_path);
                }
            }
        }
        return {}; // Path not found
    }

    // --- Move::to_text Implementation ---
    std::string Move::to_text(const Santorini::Board& board) const {
        std::string text = square_to_text(from_sq);

        switch (god) {
            case Constants::God::ARTEMIS: {
                // If not a simple adjacent move, find a plausible intermediate square
                // that respects board state (occupancy and height).
                auto blocks = board.get_blocks();
                if (!is_adjacent(from_sq, to_sq) || blocks[to_sq] - blocks[from_sq] > 1) {
                    for (sq_i mid = 0; mid < 25; ++mid) {
                        // Check geometry
                        if (mid == from_sq || mid == to_sq || !is_adjacent(from_sq, mid) || !is_adjacent(to_sq, mid)) {
                            continue;
                        }
                        // Check board state for a valid two-step path
                        bool is_valid_path = board.is_free(mid) &&
                                             (blocks[mid] - blocks[from_sq] <= 1) &&
                                             (blocks[to_sq] - blocks[mid] <= 1);

                        if (is_valid_path) {
                            text += square_to_text(mid);
                            break; // Found a valid mid-point
                        }
                    }
                }
                text += square_to_text(to_sq);
                text += square_to_text(build_sq);
                break;
            }

            case Constants::God::HERMES: {
                auto blocks = board.get_blocks();
                if (!is_adjacent(from_sq, to_sq) || blocks[to_sq] - blocks[from_sq] > 1) {
                    std::vector<sq_i> path = find_hermes_path(from_sq, to_sq, board);
                    for (const auto step: path) {
                        text += square_to_text(step);
                    }
                    text += square_to_text(build_sq);
                }
                else {
                    text += square_to_text(to_sq);
                    text += square_to_text(build_sq);
                }
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

