#include "board.h"

#include <stdexcept>

#include <algorithm>

#include <vector>

#include <deque>

#include <set>


namespace Santorini {

  // ############################################################################

  // # Constructor and Parser

  // ############################################################################

  Board::Board(const std::string & position) {

    _parse_position(position);

    _hash = _calculate_full_hash();

  }

  void Board::_parse_position(const std::string & position) {

    if (position.length() != 54) {

      throw std::invalid_argument("Invalid position: Expected length 54, got " + std::to_string(position.length()));

    }

    int num_gray_workers = 0;

    int num_blue_workers = 0;

    for (int i = 0; i < 25; ++i) {

      int height = position[2 * i] - '0';

      if (height < 0 || height > 4) throw std::invalid_argument("Invalid block height");

      _blocks[i] = height;

      char worker_code = position[2 * i + 1];

      if (worker_code == 'G') {

        if (num_gray_workers >= 2) throw std::invalid_argument("More than 2 gray workers");

        _workers[num_gray_workers++] = i;

      } else if (worker_code == 'B') {

        if (num_blue_workers >= 2) throw std::invalid_argument("More than 2 blue workers");

        _workers[2 + num_blue_workers++] = i;

      } else if (worker_code != 'N') {

        throw std::invalid_argument("Invalid worker code");

      }

    }

    if (num_gray_workers != 2 || num_blue_workers != 2) {

      throw std::invalid_argument("Invalid worker count");

    }

    if (position[50] == '0') _turn = 1;

    else if (position[50] == '1') _turn = -1;

    else throw std::invalid_argument("Invalid turn character");

    int god1_val = position[51] - '0';

    int god2_val = position[52] - '0';

    if (god1_val < 0 || god1_val > 9 || god2_val < 0 || god2_val > 9) {

      throw std::invalid_argument("Invalid god ID");

    }

    _gods[0] = static_cast < Constants::God > (god1_val);

    _gods[1] = static_cast < Constants::God > (god2_val);

    _prevent_up_next_turn = (position[53] == '1');

  }

  std::string Board::to_text() const {

    std::string pos_str;

    pos_str.reserve(54);

    std::array < char, 25 > worker_map {};

    worker_map.fill('N');

    worker_map[_workers[0]] = 'G';

    worker_map[_workers[1]] = 'G';

    worker_map[_workers[2]] = 'B';

    worker_map[_workers[3]] = 'B';

    for (int i = 0; i < 25; ++i) {

      pos_str += std::to_string(_blocks[i]);

      pos_str += worker_map[i];

    }

    pos_str += (_turn == 1 ? '0' : '1');

    pos_str += std::to_string(static_cast < int > (_gods[0]));

    pos_str += std::to_string(static_cast < int > (_gods[1]));

    pos_str += (_prevent_up_next_turn ? '1' : '0');

    return pos_str;

  }

  // ############################################################################

  // # Public Methods

  // ############################################################################

  void Board::make_move(const Moves::Move & move) {

    _last_move_height_diff = 0;

    _won = (_blocks[move.from_sq] < 3 && _blocks[move.to_sq] == 3);

    _execute_god_move(move);

    int current_player_idx = (_turn == 1) ? 0 : 1;

    if (_gods[current_player_idx] == Constants::God::ATHENA && _last_move_height_diff > 0) {

      if (!_prevent_up_next_turn) _xor_hash(Constants::ATHENA_EFFECT);

      _prevent_up_next_turn = true;

    } else {

      if (_prevent_up_next_turn) _xor_hash(Constants::ATHENA_EFFECT);

      _prevent_up_next_turn = false;

    }

    _turn *= -1;

    _xor_hash(Constants::ZOBRIST_TURN);

  }

  void Board::unmake_move(const Moves::Move & move) {

    _turn *= -1;

    _xor_hash(Constants::ZOBRIST_TURN);

    if (_prevent_up_next_turn != move.had_athena_flag) {

      _xor_hash(Constants::ATHENA_EFFECT);

    }

    _prevent_up_next_turn = move.had_athena_flag;

    _won = false;

    _undo_god_move(move);

  }

  void Board::make_null_move() {

    _xor_hash(Constants::ZOBRIST_TURN);

    _turn *= -1;

    if (_prevent_up_next_turn) {

      _xor_hash(Constants::ATHENA_EFFECT);

      _prevent_up_next_turn = false;

    }

  }

  void Board::unmake_null_move(bool prev_prevent_up_flag) {

    _xor_hash(Constants::ZOBRIST_TURN);

    _turn *= -1;

    if (prev_prevent_up_flag) {

      _xor_hash(Constants::ATHENA_EFFECT);

      _prevent_up_next_turn = true;

    }

  }

  std::vector < std::unique_ptr < Moves::Move >> Board::generate_moves() const {

    auto moves = _generate_god_moves();

    for (auto & move: moves) {

      move -> had_athena_flag = _prevent_up_next_turn;

    }

    return moves;

  }

  int Board::check_state() {

    int last_player = -_turn;

    if (_won) return last_player;

    int last_player_idx = (last_player == 1) ? 0 : 1;

    if (_last_move_height_diff <= -2 && _gods[last_player_idx] == Constants::God::PAN) {

      return last_player;

    }

    if (!_player_has_any_valid_move()) {

      return -_turn;

    }

    return 0;

  }

  // ############################################################################

  // # Hashing

  // ############################################################################

  uint64_t Board::_calculate_full_hash() const {

    uint64_t h = 0;

    for (sq_i i = 0; i < 25; ++i) {

      if (_blocks[i] > 0) {

        h ^= Constants::ZOBRIST_BLOCKS[i][_blocks[i] - 1];

      }

    }

    for (int i = 0; i < 4; ++i) {

      h ^= Constants::ZOBRIST_WORKERS[_workers[i]][_player_of_worker(i)];

    }

    if (_turn == -1) {

      h ^= Constants::ZOBRIST_TURN;

    }

    if (_prevent_up_next_turn) {

      h ^= Constants::ATHENA_EFFECT;

    }

    return h;

  }

  // ############################################################################

  // # Helper Methods

  // ############################################################################

  int Board::_player_of_worker(int worker_index) const {
    return (worker_index < 2) ? 0 : 1;
  }

  bool Board::_is_opponent_worker(int worker_index) const {
    return (_turn == 1) ? (worker_index >= 2) : (worker_index < 2);
  }

  bool Board::_is_ally_worker(int worker_index) const {
    return !_is_opponent_worker(worker_index);
  }

  std::optional < int > Board::_which_worker_is_here(sq_i s) const {

    for (int i = 0; i < 4; ++i) {

      if (_workers[i] == s) return i;

    }

    return std::nullopt;

  }

  bool Board::is_free(sq_i s) const {

    if (_blocks[s] >= 4) return false;

    for (sq_i w_pos: _workers) {

      if (w_pos == s) return false;

    }

    return true;

  }

  void Board::_move_worker(sq_i from, sq_i to) {

    for (int i = 0; i < 4; ++i) {

      if (_workers[i] == from) {

        _xor_hash(Constants::ZOBRIST_WORKERS[from][_player_of_worker(i)]);

        _xor_hash(Constants::ZOBRIST_WORKERS[to][_player_of_worker(i)]);

        _workers[i] = to;

        return;

      }

    }

  }

  void Board::_move_worker_back(int worker_idx, sq_i original_pos) {

    sq_i current_pos = _workers[worker_idx];

    _xor_hash(Constants::ZOBRIST_WORKERS[current_pos][_player_of_worker(worker_idx)]);

    _xor_hash(Constants::ZOBRIST_WORKERS[original_pos][_player_of_worker(worker_idx)]);

    _workers[worker_idx] = original_pos;

  }

  void Board::_inc_block(sq_i s) {

    int8_t h = _blocks[s];

    if (h > 0) _xor_hash(Constants::ZOBRIST_BLOCKS[s][h - 1]);

    _blocks[s]++;

    _xor_hash(Constants::ZOBRIST_BLOCKS[s][h]);

  }

  void Board::_dec_block(sq_i s) {

    int8_t h = _blocks[s];

    _xor_hash(Constants::ZOBRIST_BLOCKS[s][h - 1]);

    _blocks[s]--;

    if (_blocks[s] > 0) _xor_hash(Constants::ZOBRIST_BLOCKS[s][_blocks[s] - 1]);

  }

  void Board::_restore_block_height(sq_i s, int8_t original_height) {

    int8_t current_h = _blocks[s];

    if (current_h > 0) _xor_hash(Constants::ZOBRIST_BLOCKS[s][current_h - 1]);

    _blocks[s] = original_height;

    if (original_height > 0) _xor_hash(Constants::ZOBRIST_BLOCKS[s][original_height - 1]);

  }

  bool Board::_adj_ok(sq_i from, sq_i to) const {

    const auto & neighbours = Constants::NEIGHBOURS[from];

    return std::find(neighbours.begin(), neighbours.end(), to) != neighbours.end();

  }

  std::optional < sq_i > Board::_calculate_push_square(sq_i from_sq, sq_i to_sq) {

    // Calculate the direction vector (dx, dy)

    int dx = (to_sq % 5) - (from_sq % 5);

    int dy = (to_sq / 5) - (from_sq / 5);

    // Calculate the new coordinates for the pushed worker

    int push_r = (to_sq / 5) + dy;

    int push_c = (to_sq % 5) + dx;

    // Check if the new square is within the 5x5 board boundaries

    if (push_r >= 0 && push_r <= 4 && push_c >= 0 && push_c <= 4) {

      // If it is, return the new square index

      return static_cast < sq_i > (push_r * 5 + push_c);

    }

    // If the pushed square is off the board, return null

    return std::nullopt;

  }

  bool Board::_height_ok(sq_i from, sq_i to) const {
    return _blocks[to] - _blocks[from] <= 1;
  }

  bool Board::_move_checks(sq_i from, sq_i to) const {
    return _height_ok(from, to) && _adj_ok(from, to) && is_free(to);
  }

  bool Board::_build_ok(sq_i from, sq_i to, sq_i build) const {

    if (!_adj_ok(to, build) || to == build) return false;

    return (from == build) || is_free(build);

  }

  bool Board::_complete_checks(sq_i from, sq_i to, sq_i build) const {
    return _move_checks(from, to) && _build_ok(from, to, build);
  }

  bool Board::_player_has_any_valid_move() {

    int current_player_idx = (_turn == 1) ? 0 : 1;

    Constants::God god = _gods[current_player_idx];

    int start_idx = (_turn == 1) ? 0 : 2;

    for (int i = 0; i < 2; ++i) {

      sq_i w_pos = _workers[start_idx + i];

      for (sq_i to_sq: Constants::NEIGHBOURS[w_pos]) {

        // Cannot move to a domed square

        if (_blocks[to_sq] == 4) continue;

        int from_h = _blocks[w_pos];

        int to_h = _blocks[to_sq];

        // Hermes can move to any free adjacent square as a start for a chain move.

        // A "valid move" for Hermes only requires the initial step, not a build,

        // as the build happens after the entire chain.

        if (god == Constants::God::HERMES) {

          if (is_free(to_sq)) {

            return true;

          }

          continue; // Skip build checks for Hermes' initial move

        }

        // Cannot move up more than one level

        if (to_h - from_h > 1) continue;

        // Athena's power prevents moving up

        if (_prevent_up_next_turn && to_h > from_h) continue;

        auto occupant = _which_worker_is_here(to_sq);

        if (!occupant) { // Standard move to an empty square

          // A valid move requires a subsequent valid build

          for (sq_i build_sq: Constants::NEIGHBOURS[to_sq]) {

            if (_blocks[build_sq] < 4 && (is_free(build_sq) || build_sq == w_pos)) {

              return true;

            }

          }

        } else { // Square is occupied

          if (_is_opponent_worker( * occupant)) {

            if (god == Constants::God::APOLLO) {

              // Apollo swap: the current worker moves to `to_sq`, the opponent worker moves to `w_pos`.

              // A valid Apollo move requires a subsequent valid build from the new position (`to_sq`).

              for (sq_i build_sq: Constants::NEIGHBOURS[to_sq]) {

                // Apollo cannot build on the square the opponent worker moves to (`w_pos`),

                // nor on a domed square, nor on an occupied square (after the swap).

                if (build_sq != w_pos && _blocks[build_sq] < 4 && is_free(build_sq)) {

                  return true;

                }

              }

            } else if (god == Constants::God::MINOTAUR) {

              // Minotaur push: opponent worker is pushed to `push_sq`.

              auto push_sq_opt = _calculate_push_square(w_pos, to_sq);

              // Check if the push is valid (within board and target square is free)

              if (push_sq_opt.has_value() && is_free( * push_sq_opt)) {

                sq_i push_sq = * push_sq_opt; // Get the actual square value

                // A valid Minotaur move requires a subsequent valid build.

                // The Minotaur worker moves to `to_sq`.

                for (sq_i build_sq: Constants::NEIGHBOURS[to_sq]) {

                  // Cannot build on the square just moved from (`w_pos`),

                  // nor the square the pushed worker landed on (`push_sq`),

                  // nor a domed square, nor an occupied square.

                  if (build_sq != w_pos && build_sq != push_sq && _blocks[build_sq] < 4 && is_free(build_sq)) {

                    return true;

                  }

                }

              }

            }

          }

        }

      }

    }

    return false;

  }

  // ############################################################################

  // # God Dispatchers

  // ############################################################################

  /**
 * @brief Executes a move on the board using the unified Move object.
 *
 * This method interprets the properties of the given Move object to perform
 * the correct actions based on the specified god's power. It assumes the
 * existence of helper methods like _move_worker, _swap_workers, _build_at,
 * and board state variables like _workers, _heights, and _athena_flag.
 */
void Board::_execute_god_move(const Moves::Move& move) {
  // The Athena flag must be cleared at the start of any turn.
  // This ensures her power only lasts for one round of opponent moves.
  _prevent_up_next_turn = false;

  switch (move.god) {
    case Constants::God::APOLLO: {
      int my_worker_idx = *_which_worker_is_here(move.from_sq);

      std::optional<int> occupant_idx = _which_worker_is_here(move.to_sq);

      if (occupant_idx.has_value()) {
        _xor_hash(Constants::ZOBRIST_WORKERS[move.to_sq][_player_of_worker(*occupant_idx)]);
        _xor_hash(Constants::ZOBRIST_WORKERS[move.from_sq][_player_of_worker(*occupant_idx)]);
        _workers[*occupant_idx] = move.from_sq;
      }

      _xor_hash(Constants::ZOBRIST_WORKERS[move.from_sq][_player_of_worker(my_worker_idx)]);
      _xor_hash(Constants::ZOBRIST_WORKERS[move.to_sq][_player_of_worker(my_worker_idx)]);
      _workers[my_worker_idx] = move.to_sq;

      _inc_block(move.build_sq);

      break;
    }

    case Constants::God::MINOTAUR: {
      auto occupant_idx = _which_worker_is_here(move.to_sq);
      if (occupant_idx) {
        int dx = (move.to_sq % 5) - (move.from_sq % 5);
        int dy = (move.to_sq / 5) - (move.from_sq / 5);

        sq_i push_sq = (move.to_sq / 5 + dy) * 5 + ((move.to_sq % 5) + dx);
        _move_worker_back(*occupant_idx, push_sq);
      }

      _move_worker(move.from_sq, move.to_sq);
      _inc_block(move.build_sq);
      break;
    }

    case Constants::God::PROMETHEUS: {
      // Perform the optional pre-move build if it exists.
      if (move.extra_build_sq.has_value()) {
        _inc_block(*move.extra_build_sq);
      }
      _move_worker(move.from_sq, move.to_sq);
      _inc_block(move.build_sq);
      break;
    }

    case Constants::God::DEMETER:
    case Constants::God::HEPHAESTUS: {
      _move_worker(move.from_sq, move.to_sq);
      _inc_block(move.build_sq);
      // Perform the second build if it exists.
      if (move.extra_build_sq.has_value()) {
        _inc_block(*move.extra_build_sq);
      }
      break;
    }
    // Atlas can build a dome at any level.
    case Constants::God::ATLAS: {
      _move_worker(move.from_sq, move.to_sq);
      if (move.dome) {
        _restore_block_height(move.build_sq, 4);
      } else {
        _inc_block(move.build_sq);
      }
      break;
    }

    // Athena sets a flag if she moves up.
    case Constants::God::ATHENA: {
      _last_move_height_diff = _blocks[move.to_sq] - _blocks[move.from_sq];
      _move_worker(move.from_sq, move.to_sq);
      _inc_block(move.build_sq);
      break;
    }

    // Default case for gods with standard move-then-build mechanics.
    // This includes Artemis, Hermes, and Pan, as their special powers relate
    // to move *generation* or win *conditions*, not the execution of a single move.
    case Constants::God::ARTEMIS:
    case Constants::God::HERMES:
    case Constants::God::PAN: {
      default:
        _move_worker(move.from_sq, move.to_sq);
        _inc_block(move.build_sq);
        _last_move_height_diff = _blocks[move.to_sq] - _blocks[move.from_sq];
      break;
    }
  }
}
void Board::_undo_god_move(const Moves::Move& move) {
  // Restore the Athena flag to its state *before* the move was made.
  // This value should be stored in the Move object.
  _prevent_up_next_turn = move.had_athena_flag;

  switch (move.god) {
    case Constants::God::APOLLO: {
      _dec_block(move.build_sq);
      int my_worker_idx = *_which_worker_is_here(move.to_sq);
      auto opp_idx = _which_worker_is_here(move.from_sq);

      if (opp_idx && _is_opponent_worker(*opp_idx)) {
        _move_worker_back(*opp_idx, move.to_sq);
      }
      _move_worker_back(my_worker_idx, move.from_sq);
      break;
    }

    case Constants::God::MINOTAUR: {
      _dec_block(move.build_sq);
      int my_worker_idx = *_which_worker_is_here(move.to_sq);
      _move_worker_back(my_worker_idx, move.from_sq);

      if (move.minotaur_pushed) {
        int dx = (move.to_sq % 5) - (move.from_sq % 5);
        int dy = (move.to_sq / 5) - (move.from_sq / 5);
        sq_i push_sq = ((move.to_sq / 5) + dy) * 5 + ((move.to_sq % 5) + dx);
        int opp_idx = *_which_worker_is_here(push_sq);
        _move_worker_back(opp_idx, move.to_sq);
      }
      break;
    }

    case Constants::God::DEMETER:
    case Constants::God::HEPHAESTUS:
    case Constants::God::PROMETHEUS: {
      _dec_block(move.build_sq);
      int worker_idx = *_which_worker_is_here(move.to_sq);
      _move_worker_back(worker_idx, move.from_sq);
      if (move.extra_build_sq) _dec_block(*move.extra_build_sq);
      break;
    }

    case Constants::God::ATLAS: {
      _restore_block_height(move.build_sq, *move.atlas_original_height);
      int worker_idx = *_which_worker_is_here(move.to_sq);
      _move_worker_back(worker_idx, move.from_sq);
      break;
    }

    case Constants::God::ATHENA: {
      _dec_block(move.build_sq);
      int worker_idx = *_which_worker_is_here(move.to_sq);
      _move_worker_back(worker_idx, move.from_sq);
      _last_move_height_diff = 0;
      break;
    }

    case Constants::God::ARTEMIS:
    case Constants::God::HERMES:
    case Constants::God::PAN:
    default: {
      _dec_block(move.build_sq);
      int worker_idx = *_which_worker_is_here(move.to_sq);
      _move_worker_back(worker_idx, move.from_sq);
      _last_move_height_diff = 0;
      break;
    }
  }
}

bool Board::_blocked_by_athena(const int from_sq, const int to_sq) const {

  return _prevent_up_next_turn && _blocks[to_sq] > _blocks[from_sq];

}

std::vector < std::unique_ptr < Moves::Move >> Board::_generate_god_moves() const {

    int current_player_idx = (_turn == 1) ? 0 : 1;

    switch (_gods[current_player_idx]) {

    case Constants::God::APOLLO:
      return _generate_apollo_moves();

    case Constants::God::ARTEMIS:
      return _generate_artemis_moves();

    case Constants::God::ATHENA:
      return _generate_athena_moves();

    case Constants::God::ATLAS:
      return _generate_atlas_moves();

    case Constants::God::DEMETER:
      return _generate_demeter_moves();

    case Constants::God::HEPHAESTUS:
      return _generate_hephaestus_moves();

    case Constants::God::HERMES:
      return _generate_hermes_moves();

    case Constants::God::MINOTAUR:
      return _generate_minotaur_moves();

    case Constants::God::PAN:
      return _generate_pan_moves();

    case Constants::God::PROMETHEUS:
      return _generate_prometheus_moves();

    }
    return {};

  }

  std::vector<std::unique_ptr<Moves::Move>> Board::_generate_apollo_moves() const {
    std::vector<std::unique_ptr<Moves::Move>> moves;
    int start_idx = (_turn == 1) ? 0 : 2;
    for (int i = 0; i < 2; ++i) {

      sq_i from_sq = _workers[start_idx + i];

      for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {

        if (_blocked_by_athena(from_sq, to_sq)) continue;
        if (_blocks[to_sq] - _blocks[from_sq] > 1) continue;

        auto occupant = _which_worker_is_here(to_sq);

        if (_blocks[to_sq] == 4 || (occupant && _is_ally_worker(*occupant))) continue;

        for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
          if (build_sq == to_sq) continue;
          if (build_sq == from_sq) {
            if (occupant || _blocks[build_sq] == 4) continue;
          } else if (!is_free(build_sq)) continue;
            moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::APOLLO));
        }
      }
    }
    return moves;
  }


  std::vector<std::unique_ptr<Moves::Move>> Board::_generate_artemis_moves() const {
    std::vector<std::unique_ptr<Moves::Move>> moves;
    int start_idx = (_turn == 1) ? 0 : 2;

    for (int i = 0; i < 2; ++i) {
      sq_i from_sq = _workers[start_idx + i];

      for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
        if (_blocked_by_athena(from_sq, to_sq)) continue;
        if (!_move_checks(from_sq, to_sq)) continue;

        for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
          if (_build_ok(from_sq, to_sq, build_sq)) {
            moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::ARTEMIS));
          }
        }

        for (sq_i second_sq : Constants::NEIGHBOURS[to_sq]) {
          if (second_sq == from_sq || !_move_checks(to_sq, second_sq) ||
          _blocked_by_athena(to_sq, second_sq)) continue;

          for (sq_i build_sq : Constants::NEIGHBOURS[second_sq]) {
            if (_build_ok(from_sq, second_sq, build_sq)) {
              moves.push_back(std::make_unique<Moves::Move>(from_sq, second_sq, build_sq, Constants::God::ARTEMIS));
            }
          }
        }
      }
    }
    return moves;
  }


// --- Athena ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_athena_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (!_move_checks(from_sq, to_sq)) continue;

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_build_ok(from_sq, to_sq, build_sq)) {
          moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::ATHENA));
        }
      }
    }
  }
  return moves;
}

// --- Atlas ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_atlas_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (_blocked_by_athena(from_sq, to_sq)) continue;
      if (!_move_checks(from_sq, to_sq)) continue;

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_build_ok(from_sq, to_sq, build_sq)) {
          // Normal build
          auto move = std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::ATLAS);
          move->atlas_original_height = _blocks[build_sq];
          moves.push_back(std::move(move));
          // Dome build
          if (_blocks[build_sq] < 4) {
            auto dome_move = std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::ATLAS);
            dome_move->dome = true;
            dome_move->atlas_original_height = _blocks[build_sq];
            moves.push_back(std::move(dome_move));
          }
        }
      }
    }
  }
  return moves;
}

// --- Demeter ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_demeter_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (_blocked_by_athena(from_sq, to_sq)) continue;
      if (!_move_checks(from_sq, to_sq)) continue;
      std::vector<sq_i> build_sqs;

      for(sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_build_ok(from_sq, to_sq, build_sq)) {
          build_sqs.push_back(build_sq);
        }
      }

      for (size_t j = 0; j < build_sqs.size(); ++j) {
        moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sqs[j], Constants::God::DEMETER));

        for (size_t k = j + 1; k < build_sqs.size(); ++k) {
          auto move = std::make_unique<Moves::Move>(from_sq, to_sq, build_sqs[j], Constants::God::DEMETER);
          move->extra_build_sq = build_sqs[k];
          moves.push_back(std::move(move));
        }
      }
    }
  }
  return moves;
}

// --- Hephaestus ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_hephaestus_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (_blocked_by_athena(from_sq, to_sq)) continue;
      if (!_move_checks(from_sq, to_sq)) continue;

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_build_ok(from_sq, to_sq, build_sq)) {
          moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::HEPHAESTUS));
          if (_blocks[build_sq] < 2) {
            auto move = std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::HEPHAESTUS);
            move->extra_build_sq = build_sq;
            moves.push_back(std::move(move));
          }
        }
      }
    }
  }
  return moves;
}

// --- Hermes ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_hermes_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];
    int8_t h = _blocks[from_sq];

    // Standard moves
    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (!_move_checks(from_sq, to_sq)) continue;

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_blocked_by_athena(from_sq, to_sq)) continue;
        if (_build_ok(from_sq, to_sq, build_sq)) {
          moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::HERMES));
        }
      }
    }

    // Build without moving
    for (sq_i build_sq : Constants::NEIGHBOURS[from_sq]) {
      if (_build_ok(from_sq, from_sq, build_sq)) {
        moves.push_back(std::make_unique<Moves::Move>(from_sq, from_sq, build_sq, Constants::God::HERMES));
      }
    }
    // Multi-step ground moves
    std::set<sq_i> visited;
    std::deque<sq_i> q;
    q.push_back(from_sq);
    visited.insert(from_sq);
    while(!q.empty()) {
      sq_i curr = q.front();
      q.pop_front();

      for (sq_i next_sq : Constants::NEIGHBOURS[curr]) {
        if (visited.count(next_sq) || !is_free(next_sq) || _blocks[next_sq] != h) continue;
        visited.insert(next_sq);

        for (sq_i build_sq : Constants::NEIGHBOURS[next_sq]) {
          if (_build_ok(from_sq, next_sq, build_sq)) {
            moves.push_back(std::make_unique<Moves::Move>(from_sq, next_sq, build_sq, Constants::God::HERMES));
          }
        }
        q.push_back(next_sq);
      }
    }
  }
  return moves;
}

// --- Minotaur ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_minotaur_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (_blocks[to_sq] - _blocks[from_sq] > 1) continue;
      if (_blocked_by_athena(from_sq, to_sq)) continue;
      auto occupant = _which_worker_is_here(to_sq);
      if (_blocks[to_sq] == 4 || (occupant && _is_ally_worker(*occupant))) continue;
      std::optional<sq_i> push_sq;
      if (occupant && _is_opponent_worker(*occupant)) {
        int dx = (to_sq % 5) - (from_sq % 5);
        int dy = (to_sq / 5) - (from_sq / 5);
        int push_col = (to_sq % 5) + dx;
        int push_row = (to_sq / 5) + dy;
        if (push_row < 0 || push_row > 4 || push_col < 0 || push_col > 4) continue;
        push_sq = push_row * 5 + push_col;
        if (!is_free(*push_sq)) continue;
      }

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (push_sq && *push_sq == build_sq) continue;
        if (_build_ok(from_sq, to_sq, build_sq)) {
          auto move = std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::MINOTAUR);
          move->minotaur_pushed = push_sq.has_value();
          moves.push_back(std::move(move));
        }
      }
    }
  }
  return moves;
}

// --- Pan ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_pan_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (_blocked_by_athena(from_sq, to_sq)) continue;
      if (!_move_checks(from_sq, to_sq)) continue;

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_build_ok(from_sq, to_sq, build_sq)) {
          moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::PAN));
        }
      }
    }
  }
  return moves;
}

// --- Prometheus ---
std::vector<std::unique_ptr<Moves::Move>> Board::_generate_prometheus_moves() const {
  std::vector<std::unique_ptr<Moves::Move>> moves;
  int start_idx = (_turn == 1) ? 0 : 2;

  for (int i = 0; i < 2; ++i) {
    sq_i from_sq = _workers[start_idx + i];
    // Generate moves without pre-build

    for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
      if (!_move_checks(from_sq, to_sq)) continue;
      if (_blocked_by_athena(from_sq, to_sq)) continue;

      for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
        if (_build_ok(from_sq, to_sq, build_sq)) {
          moves.push_back(std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::PROMETHEUS));
        }
      }
    }

    // Generate moves with pre-build
    for (sq_i opt_build_sq : Constants::NEIGHBOURS[from_sq]) {
      if (!_build_ok(from_sq, from_sq, opt_build_sq)) continue;

      for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
        int temp_h_adj = (to_sq == opt_build_sq) ? 1 : 0;
        if (_blocks[to_sq] + temp_h_adj > _blocks[from_sq]) continue;
        if (!is_free(to_sq) && to_sq != from_sq) continue;
        if (!_adj_ok(from_sq, to_sq)) continue;

        for (sq_i build_sq : Constants::NEIGHBOURS[to_sq]) {
          if (_build_ok(from_sq, to_sq, build_sq)) {
            auto move = std::make_unique<Moves::Move>(from_sq, to_sq, build_sq, Constants::God::PROMETHEUS);
            move->extra_build_sq = opt_build_sq;
            moves.push_back(std::move(move));
          }
        }
      }
    }
  }
  return moves;
}

  bool operator < (const Board & lhs,
    const Board & rhs) {

    return lhs._hash < rhs._hash;

  }

} // namespace Santorini