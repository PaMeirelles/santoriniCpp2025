#pragma once

#include <string>
#include <stdexcept>
#include <optional>
#include "constants.h"
#include <queue>

namespace Santorini {
    class Board;
}

using namespace std;

// Helper to get (row, col) coordinates from a square index
inline pair<int, int> get_coords(sq_i square) {
    return { square % 5, square / 5 };
}

// Helper to check if two squares are adjacent (including diagonals)
inline bool is_adjacent(sq_i s1, sq_i s2) {
    if (s1 == s2) return false;
    auto [r1, c1] = get_coords(s1);
    auto [r2, c2] = get_coords(s2);
    return abs(r1 - r2) <= 1 && abs(c1 - c2) <= 1;
}

/**
 * @brief Finds the shortest path for a Hermes move on an empty grid.
 *
 * Uses Breadth-First Search (BFS) to find a shortest sequence of squares
 * from a starting to an ending position.
 * @param from The starting square.
 * @param to The final square.
 * @return A vector of square indices representing the path steps *after* 'from'.
 * Returns an empty vector if from == to.
 */
inline vector<sq_i> find_hermes_path(sq_i from, sq_i to) {
    if (from == to) {
        return {};
    }

    queue<vector<sq_i>> q;
    q.push({from});

    vector<bool> visited(25, false);
    visited[from] = true;

    while (!q.empty()) {
        vector<sq_i> path = q.front();
        q.pop();
        sq_i current = path.back();

        if (current == to) {
            path.erase(path.begin()); // Remove the 'from' square
            return path;
        }

        // Explore neighbors
        auto [r, c] = get_coords(current);
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;

                int nr = r + dr;
                int nc = c + dc;

                if (nr >= 0 && nr < 5 && nc >= 0 && nc < 5) {
                    sq_i neighbor = nc * 5 + nr;
                    if (!visited[neighbor]) {
                        visited[neighbor] = true;
                        vector<sq_i> new_path = path;
                        new_path.push_back(neighbor);
                        q.push(new_path);
                    }
                }
            }
        }
    }
    return {};
}

namespace Santorini::Moves {

  // #############################################################################

  // # Utility Functions

  // #############################################################################

  inline sq_i text_to_square(const string & square_text) {
    if (square_text.length() != 2) {
      throw invalid_argument("Square text must be 2 characters long: " + square_text);
    }
    int row = square_text[0] - 'a';
    int col = square_text[1] - '1';
    if (row < 0 || row > 4 || col < 0 || col > 4) {
      throw invalid_argument("Invalid square format: " + square_text);
    }
    return static_cast < sq_i > (col * 5 + row);
  }

  inline string square_to_text(sq_i square) {
    if (square < 0 || square > 24) {
      throw out_of_range("Square index out of range: " + to_string(square));
    }
    char row = (square % 5) + 'a';
    char col = (square / 5) + '1';
    return {
      row,
      col
    };
  }

  // #############################################################################

  // # Abstract Base Class

  // #############################################################################

  class Move {

    public:
      // Everyone
      sq_i from_sq;
      bool had_athena_flag = false;
      int score = 0;
      Constants::God god;
      sq_i to_sq;
      sq_i build_sq;

      // Some gods
      optional<sq_i> extra_build_sq;
      bool dome = false;
      // Reversibility
      optional<int> atlas_original_height;
      bool minotaur_pushed = false;

    Move(const sq_i from_sq,
      const sq_i to_sq,
      const sq_i build_sq,
      const Constants::God god): from_sq(from_sq), god(god), to_sq(to_sq), build_sq(build_sq) {
      }
      [[nodiscard]] std::string to_text(const Santorini::Board& board) const;
  };

} // namespace Santorini::Moves