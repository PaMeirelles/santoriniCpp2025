#pragma once

#include <optional>
#include <cstdint> // Required for uint32_t
#include "constants.h" // Assumes constants.h is in the include path
#include <string>     // Required for std::string
#include <sstream>    // Required for std::stringstream

namespace Santorini {

// Forward-declare the Board class to avoid a circular dependency.
class Board;

namespace Moves {

  // A move represented as a single 32-bit unsigned integer for efficiency.
  using moveByte = uint32_t;
  // #############################################################################
  // # Constants for Bitpacking
  // #############################################################################

  // Sentinel value for an empty optional height. 7 is outside the 0-4 range.
  constexpr int NO_HEIGHT = 7;

  // --- Bit Shifts ---
  constexpr int FROM_SQ_SHIFT = 0;
  constexpr int TO_SQ_SHIFT = 5;
  constexpr int BUILD_SQ_SHIFT = 10;
  constexpr int EXTRA_BUILD_SQ_SHIFT = 15;
  constexpr int GOD_SHIFT = 20;
  constexpr int PREV_H_SHIFT = 24;
  constexpr int DOME_SHIFT = 27;
  constexpr int MINOTAUR_PUSHED_SHIFT = 28;
  constexpr int HAD_ATHENA_FLAG_SHIFT = 29;
  constexpr int WIN_SHIFT = 30;

  // --- Bit Masks ---
  constexpr uint32_t FROM_SQ_MASK = 0x1FU; // 5 bits
  constexpr uint32_t TO_SQ_MASK = 0x1FU;
  constexpr uint32_t BUILD_SQ_MASK = 0x1FU;
  constexpr uint32_t EXTRA_BUILD_SQ_MASK = 0x1FU;
  constexpr uint32_t GOD_MASK = 0xFU; // 4 bits
  constexpr uint32_t PREV_H_MASK = 0x7U; // 3 bits
  constexpr uint32_t DOME_MASK = 0x1U; // 1 bit
  constexpr uint32_t MINOTAUR_PUSHED_MASK = 0x1U;
  constexpr uint32_t HAD_ATHENA_FLAG_MASK = 0x1U;
  constexpr uint32_t WIN_MASK = 0x1U;

  typedef struct Move {
    moveByte move;
    int score;

    bool operator==(const Move & other) const {
      // Create a mask to zero out the prev_h bits.
      constexpr uint32_t comparison_mask = ~(PREV_H_MASK << PREV_H_SHIFT);
      // Apply the mask to both sides before comparing.
      return (move & comparison_mask) == (other.move & comparison_mask);
    }
  } Move;

  inline std::string square_to_text(sq_i square) {
    if (square < 0 || square > 24) {
      throw std::out_of_range("Square index out of range: " + std::to_string(square));
    }
    char row = (square % 5) + 'a';
    char col = (square / 5) + '1';
    return {row, col};
  }

  // #############################################################################
  // # "Constructor" (Packing Function)
  // #############################################################################

  /**
   * @brief Packs move data into a 32-bit integer.
   */
  inline Move create_move(
    const sq_i from_sq,
    const sq_i to_sq,
    const sq_i build_sq,
    const Constants::God god,
    const bool is_win = false,
    const bool is_dome = false,
    const std::optional < sq_i > & extra_build_sq = std::nullopt,
    const std::optional < int > & atlas_original_height = std::nullopt,
    const bool minotaur_pushed = false,
    const bool had_athena_flag = false
  ) {
    uint32_t move_data = 0;

    move_data |= (static_cast < uint32_t > (from_sq) & FROM_SQ_MASK) << FROM_SQ_SHIFT;
    move_data |= (static_cast < uint32_t > (to_sq) & TO_SQ_MASK) << TO_SQ_SHIFT;
    move_data |= (static_cast < uint32_t > (build_sq) & BUILD_SQ_MASK) << BUILD_SQ_SHIFT;
    move_data |= (static_cast < uint32_t > (god) & GOD_MASK) << GOD_SHIFT;

    // Handle optional and boolean values
    const uint32_t extra_sq = extra_build_sq.value_or(Constants::NO_SQ);
    move_data |= (extra_sq & EXTRA_BUILD_SQ_MASK) << EXTRA_BUILD_SQ_SHIFT;

    const uint32_t prev_h = atlas_original_height.value_or(NO_HEIGHT);
    move_data |= (prev_h & PREV_H_MASK) << PREV_H_SHIFT;

    if (is_dome) move_data |= (1U << DOME_SHIFT);
    if (minotaur_pushed) move_data |= (1U << MINOTAUR_PUSHED_SHIFT);
    if (had_athena_flag) move_data |= (1U << HAD_ATHENA_FLAG_SHIFT);
    if (is_win) move_data |= (1U << WIN_SHIFT);

    Move move;
    move.move = move_data;
    move.score = 0;

    return move;
  }

  // #############################################################################
  // # "Getters" (Unpacking Functions)
  // #############################################################################

  [[nodiscard]] inline sq_i get_from_sq(const moveByte move) {
    return static_cast < sq_i > ((move >> FROM_SQ_SHIFT) & FROM_SQ_MASK);
  }

  [[nodiscard]] inline sq_i get_to_sq(const moveByte move) {
    return static_cast < sq_i > ((move >> TO_SQ_SHIFT) & TO_SQ_MASK);
  }

  [[nodiscard]] inline sq_i get_build_sq(const moveByte move) {
    return static_cast < sq_i > ((move >> BUILD_SQ_SHIFT) & BUILD_SQ_MASK);
  }

  [[nodiscard]] inline Constants::God get_god(const moveByte move) {
    return static_cast < Constants::God > ((move >> GOD_SHIFT) & GOD_MASK);
  }

  [[nodiscard]] inline bool is_win(const moveByte move) {
    return ((move >> WIN_SHIFT) & WIN_MASK) != 0;
  }

  [[nodiscard]] inline bool is_dome(const moveByte move) {
    return ((move >> DOME_SHIFT) & DOME_MASK) != 0;
  }

  [[nodiscard]] inline std::optional < sq_i > get_extra_build_sq(const moveByte move) {
    const sq_i sq = static_cast < sq_i > ((move >> EXTRA_BUILD_SQ_SHIFT) & EXTRA_BUILD_SQ_MASK);
    if (sq == Constants::NO_SQ) {
      return std::nullopt;
    }
    return sq;
  }

  [[nodiscard]] inline std::optional < int > get_prev_height(const moveByte move) {
    const int height = static_cast < int > ((move >> PREV_H_SHIFT) & PREV_H_MASK);
    if (height == NO_HEIGHT) {
      return std::nullopt;
    }
    return height;
  }

  [[nodiscard]] inline bool was_minotaur_pushed(const moveByte move) {
    return ((move >> MINOTAUR_PUSHED_SHIFT) & MINOTAUR_PUSHED_MASK) != 0;
  }

  [[nodiscard]] inline bool had_athena_flag(const moveByte move) {
    return ((move >> HAD_ATHENA_FLAG_SHIFT) & HAD_ATHENA_FLAG_MASK) != 0;
  }

  inline void set_had_athena_flag(moveByte & move, const bool value) {
    if (value) {
      // Set the bit to 1 using bitwise OR
      move |= (1U << HAD_ATHENA_FLAG_SHIFT);
    } else {
      // Clear the bit to 0 using bitwise AND with an inverted mask
      move &= ~(1U << HAD_ATHENA_FLAG_SHIFT);
    }
  }

  inline void set_extra_build_sq(moveByte& move, const std::optional<sq_i>& extra_sq) {
    // First, clear the existing 5 bits for the extra build square
    move &= ~(EXTRA_BUILD_SQ_MASK << EXTRA_BUILD_SQ_SHIFT);

    // Get the value to pack, defaulting to NO_SQ if the optional is empty
    const uint32_t sq_to_pack = extra_sq.value_or(Constants::NO_SQ);

    // Set the new value in the correct position
    move |= (sq_to_pack & EXTRA_BUILD_SQ_MASK) << EXTRA_BUILD_SQ_SHIFT;
  }

  inline void set_dome(moveByte & move, const bool value) {
    if (value) {
      // Set the bit to 1 using bitwise OR
      move |= (1U << DOME_SHIFT);
    } else {
      // Clear the bit to 0 using bitwise AND with an inverted mask
      move &= ~(1U << DOME_SHIFT);
    }
  }

  inline void set_prev_height(moveByte & move,
    const std::optional < int > & height) {
    // First, clear the existing 3 bits for the height
    move &= ~(PREV_H_MASK << PREV_H_SHIFT);

    // Get the value to pack, defaulting to NO_HEIGHT if the optional is empty
    const uint32_t height_to_pack = height.value_or(NO_HEIGHT);

    // Set the new value in the correct position
    move |= (height_to_pack & PREV_H_MASK) << PREV_H_SHIFT;
  }

  inline void set_minotaur_push(moveByte & move, const bool value) {
    if (value) {
      // Set the bit to 1 using bitwise OR
      move |= (1U << MINOTAUR_PUSHED_SHIFT);
    } else {
      // Clear the bit to 0 using bitwise AND with an inverted mask
      move &= ~(1U << MINOTAUR_PUSHED_SHIFT);
    }
  }
  std::string move_to_text(const Board& board, Move move);
  bool is_adjacent(sq_i s1, sq_i s2);

} // namespace Santorini::Moves
} // namespace Santorini