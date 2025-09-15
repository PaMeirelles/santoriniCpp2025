#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <optional>
#include <numeric>
#include <utility>

#include "constants.h" // Assumes constants.h is in the include path

namespace Santorini {

// Forward-declare the Board class to avoid a circular dependency.
class Board;

namespace Moves {

  // #############################################################################
  // # Utility Functions
  // #############################################################################

  inline sq_i text_to_square(const std::string & square_text) {
    if (square_text.length() != 2) {
      throw std::invalid_argument("Square text must be 2 characters long: " + square_text);
    }
    int row = square_text[0] - 'a';
    int col = square_text[1] - '1';
    if (row < 0 || row > 4 || col < 0 || col > 4) {
      throw std::invalid_argument("Invalid square format: " + square_text);
    }
    return static_cast < sq_i > (col * 5 + row);
  }

  inline std::string square_to_text(sq_i square) {
    if (square < 0 || square > 24) {
      throw std::out_of_range("Square index out of range: " + std::to_string(square));
    }
    char row = (square % 5) + 'a';
    char col = (square / 5) + '1';
    return {
      row,
      col
    };
  }

  // Declaration for pathfinding helper, which needs the Board definition in the .cpp file.
  std::vector<sq_i> find_hermes_path(sq_i from, sq_i to, const Santorini::Board& board);
  bool is_adjacent(sq_i s1, sq_i s2);

  // #############################################################################
  // # The Unified Move Class
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
      bool dome = false;

      // Some gods
      std::optional<sq_i> extra_build_sq;

      // Reversibility
      std::optional<int> atlas_original_height;
      bool minotaur_pushed = false;

    Move(const sq_i from_sq,
      const sq_i to_sq,
      const sq_i build_sq,
      const Constants::God god): from_sq(from_sq), god(god), to_sq(to_sq), build_sq(build_sq) {
      }
      [[nodiscard]] std::string to_text(const Santorini::Board& board) const;

    bool operator==(const Move& other) const {
      return from_sq == other.from_sq &&
             to_sq == other.to_sq &&
             build_sq == other.build_sq &&
             dome == other.dome &&
             extra_build_sq == other.extra_build_sq;
    }
    bool operator<(const Move& other) const {
      return score > other.score;
    }
  };

} // namespace Santorini::Moves
} // namespace Santorini

