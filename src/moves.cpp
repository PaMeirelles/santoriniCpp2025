#include "moves.h"
#include "board.h" // It is now safe to include board.h here

#include <queue>

namespace Santorini::Moves {

    // --- Utility Function Implementations ---
    inline std::pair<int, int> get_coords(sq_i square) {
        return { square % 5, square / 5 };
    }

    bool is_adjacent(sq_i s1, sq_i s2) {
        if (s1 == s2) return false;
        auto [r1, c1] = get_coords(s1);
        auto [r2, c2] = get_coords(s2);
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

  std::string move_to_text(const Board& board, const Move move) {
    // Extract all move data first for clarity
    const auto from_sq = get_from_sq(move.move);
    const auto to_sq = get_to_sq(move.move);
    const auto build_sq = get_build_sq(move.move);
    const auto god = get_god(move.move);
    const auto dome = is_dome(move.move);
    const auto extra_build_sq = get_extra_build_sq(move.move);

    std::string text = square_to_text(from_sq);

    switch (god) {
        case Constants::God::ARTEMIS: {
            // Check if it was a double-move by checking adjacency and height difference
            if (!is_adjacent(from_sq, to_sq)) {
                // Find a plausible intermediate square for the text representation
                for (sq_i mid = 0; mid < 25; ++mid) {
                    if (mid == from_sq || mid == to_sq || !is_adjacent(from_sq, mid) || !is_adjacent(to_sq, mid)) {
                        continue;
                    }
                    // A simple check is usually sufficient for notation, as we assume a legal move
                    if (board.is_free(mid)) {
                        text += square_to_text(mid);
                        break;
                    }
                }
            }
            text += square_to_text(to_sq);
            text += square_to_text(build_sq);
            break;
        }

        case Constants::God::HERMES: {
             if (!is_adjacent(from_sq, to_sq)) {
                std::vector<sq_i> path = find_hermes_path(from_sq, to_sq, board);
                // The path from the helper includes the start square, so skip it
                for (size_t i = 1; i < path.size(); ++i) {
                    text += square_to_text(path[i]);
                }
             } else {
                text += square_to_text(to_sq);
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

