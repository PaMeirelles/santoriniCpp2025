#include "board.h"
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <deque>
#include <iostream>
#include <set>

namespace Santorini {

// ############################################################################
// # Constructor and Parser
// ############################################################################

Board::Board(const std::string &position) {
    _parse_position(position);

    // Check if the starting player is Prometheus to set the correct initial phase
    _current_phase = 0;
    _hash = _calculate_full_hash();
}

void Board::_parse_position(const std::string &position) {
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

    _gods[0] = static_cast<Constants::God>(god1_val);
    _gods[1] = static_cast<Constants::God>(god2_val);
    _prevent_up_next_turn = (position[53] == '1');

    _workers_map.fill(-1);
    for (int i = 0; i < 4; i++) {
        _workers_map[_workers[i]] = i;
    }
}

std::string Board::to_text() const {
    std::string pos_str;
    pos_str.reserve(54);
    std::array<char, 25> worker_map{};
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
    pos_str += std::to_string(static_cast<int>(_gods[0]));
    pos_str += std::to_string(static_cast<int>(_gods[1]));
    pos_str += (_prevent_up_next_turn ? '1' : '0');
    return pos_str;
}

bool Board::_sanity_check_workers() const {
    for (int i = 0; i < 4; i++) {
        if (_workers_map[_workers[i]] != i) return false;
    }
    auto counter = 0;
    for (int i = 0; i < 25; i++) {
        counter += _workers_map[i];
    }
    if (counter != -15) return false;
    return true;
}


// ############################################################################
// # State Machine Logic
// ############################################################################

void Board::make_action(int action_idx) {
    int current_player_idx = (_turn == 1) ? 0 : 1;
    Constants::God god = _gods[current_player_idx];

    if (_current_phase == 0) {
        // --- PHASE 0: MOVEMENT ---
        int move_val = action_idx - MOVE_OFFSET;
        sq_i to_sq = move_val % 25;
        sq_i from_sq = move_val / 25;

        _last_move_height_diff = _blocks[to_sq] - _blocks[from_sq];
        _start_sq_of_turn = from_sq;
        _active_worker = to_sq;

        auto occupant = _which_worker_is_here(to_sq);

        if (god == Constants::God::APOLLO && occupant.has_value()) {
            _swap_workers(from_sq, to_sq);
        } else if (god == Constants::God::MINOTAUR && occupant.has_value()) {
            auto push_sq = _calculate_push_square(from_sq, to_sq).value();
            _move_worker(to_sq, push_sq);
            _move_worker(from_sq, to_sq);
        } else {
            // Standard move (also handles Artemis/Hermes teleporting)
            _move_worker(from_sq, to_sq);
        }

        // Check Win Condition
        if (_blocks[from_sq] < 3 && _blocks[to_sq] == 3) {
            _won = true;
        } else if (god == Constants::God::PAN && _last_move_height_diff <= -2) {
            _won = true;
        }

        _current_phase = 1;

    } else if (_current_phase == 1) {
        // --- PHASE 1: FIRST BUILD ---
        sq_i build_sq = action_idx - BUILD_1_OFFSET;
        _inc_block(build_sq);
        _last_build_sq = build_sq;

        if (god == Constants::God::DEMETER || god == Constants::God::HEPHAESTUS || god == Constants::God::ATLAS) {
            _current_phase = 2;
        } else {
            _end_turn();
        }

    } else if (_current_phase == 2) {
        // --- PHASE 2: SECOND BUILD / DOME / PASS ---
        if (god == Constants::God::PROMETHEUS) {
            if (action_idx != PASS_ACTION) {
                sq_i build_sq = action_idx - BUILD_2_OFFSET;
                _inc_block(build_sq);
                _prometheus_built_early = true;
            }
            _current_phase = 0; // Transition to standard Move phase
        } else {
            if (action_idx != PASS_ACTION) {
                if (god == Constants::God::ATLAS && action_idx == DOME_ACTION) {
                    _restore_block_height(_last_build_sq, 4);
                } else {
                    sq_i build_sq = action_idx - BUILD_2_OFFSET;
                    _inc_block(build_sq);
                }
            }
            _end_turn();
        }
    }

    // Rather than tracking complex XORs across intermediate sub-states,
    // recalculating guarantees no transposition collisions.
    _hash = _calculate_full_hash();
}

void Board::_end_turn() {
    int current_player_idx = (_turn == 1) ? 0 : 1;

    // Athena Effect
    if (_gods[current_player_idx] == Constants::God::ATHENA && _last_move_height_diff > 0) {
        _prevent_up_next_turn = true;
    } else {
        _prevent_up_next_turn = false;
    }

    _turn *= -1;
    int next_player_idx = (_turn == 1) ? 0 : 1;

    // Reset Trackers
    _active_worker = -1;
    _start_sq_of_turn = -1;
    _last_build_sq = -1;
    _prometheus_built_early = false;
    _last_move_height_diff = 0;

    _current_phase = 0;

}

std::vector<int> Board::generate_legal_actions() const {
    std::vector<int> actions;
    int current_player_idx = (_turn == 1) ? 0 : 1;
    Constants::God god = _gods[current_player_idx];
    int start_idx = (_turn == 1) ? 0 : 2;

    if (_current_phase == 0) {
        // --- PHASE 0: MOVEMENT ---
        for (int i = 0; i < 2; ++i) {
            sq_i from_sq = _workers[start_idx + i];

            // Standard God Reachability
            for (sq_i to_sq : Constants::NEIGHBOURS[from_sq]) {
                if (_blocked_by_athena(from_sq, to_sq)) continue;

                if (god == Constants::God::APOLLO) {
                    if (_blocks[to_sq] - _blocks[from_sq] > 1) continue;
                    if (_blocks[to_sq] == 4) continue;
                    auto occupant = _which_worker_is_here(to_sq);
                    if (occupant && _is_ally_worker(*occupant)) continue;
                    actions.push_back(MOVE_OFFSET + from_sq * 25 + to_sq);
                }
                else if (god == Constants::God::MINOTAUR) {
                    if (_blocks[to_sq] - _blocks[from_sq] > 1) continue;
                    if (_blocks[to_sq] == 4) continue;
                    auto occupant = _which_worker_is_here(to_sq);
                    if (occupant && _is_ally_worker(*occupant)) continue;

                    if (occupant && _is_opponent_worker(*occupant)) {
                        auto push_sq = _calculate_push_square(from_sq, to_sq);
                        if (!push_sq || !is_free(*push_sq)) continue;
                    }
                    actions.push_back(MOVE_OFFSET + from_sq * 25 + to_sq);
                }
                else {
                    // Standard rules for everyone else (including Prometheus)
                    if (!move_checks(from_sq, to_sq)) continue;
                    if (god == Constants::God::PROMETHEUS && _prometheus_built_early && _blocks[to_sq] > _blocks[from_sq]) continue;

                    actions.push_back(MOVE_OFFSET + from_sq * 25 + to_sq);
                }
            }

            // Extended Reachability for Artemis
            if (god == Constants::God::ARTEMIS) {
                for (sq_i step1 : Constants::NEIGHBOURS[from_sq]) {
                    if (move_checks(from_sq, step1) && !_blocked_by_athena(from_sq, step1)) {
                        for (sq_i to_sq : Constants::NEIGHBOURS[step1]) {
                            if (to_sq != from_sq && move_checks(step1, to_sq) && !_blocked_by_athena(step1, to_sq)) {
                                actions.push_back(MOVE_OFFSET + from_sq * 25 + to_sq);
                            }
                        }
                    }
                }
            }

            // Extended Reachability for Hermes
            if (god == Constants::God::HERMES) {
                std::array<bool, 25> visited{};
                std::deque<sq_i> q;
                q.push_back(from_sq);
                visited[from_sq] = true;

                while(!q.empty()) {
                    sq_i curr = q.front(); q.pop_front();
                    for (sq_i next_sq : Constants::NEIGHBOURS[curr]) {
                        if (!visited[next_sq] && is_free(next_sq) && _blocks[next_sq] == _blocks[from_sq]) {
                            visited[next_sq] = true;
                            actions.push_back(MOVE_OFFSET + from_sq * 25 + next_sq);
                            q.push_back(next_sq);
                        }
                    }
                }
            }
        }

        // Remove duplicates caused by multi-path routing (Artemis/Hermes)
        std::sort(actions.begin(), actions.end());
        actions.erase(std::unique(actions.begin(), actions.end()), actions.end());

    } else if (_current_phase == 1) {
        // --- PHASE 1: FIRST BUILD ---
        sq_i from_sq = _active_worker;
        for (sq_i build_sq : Constants::NEIGHBOURS[from_sq]) {
            if (_build_ok(_start_sq_of_turn, from_sq, build_sq)) {
                if (god == Constants::God::APOLLO && _start_sq_of_turn == build_sq) {
                    continue;
                }
                if (god == Constants::God::MINOTAUR) {
                    auto push_sq = _calculate_push_square(_start_sq_of_turn, from_sq);
                    if (build_sq == push_sq) {
                        continue;
                    }
                }
                actions.push_back(BUILD_1_OFFSET + build_sq);
            }
        }

    } else if (_current_phase == 2) {
        // --- PHASE 2: SECOND BUILD / DOME / PASS ---
        if (_start_sq_of_turn == -1) std::cout << "here" << std::endl;
        if (god == Constants::God::PROMETHEUS) {
            actions.push_back(PASS_ACTION);
            for (sq_i build_sq : Constants::NEIGHBOURS[_start_sq_of_turn]) {
                if (_build_ok(_start_sq_of_turn, _start_sq_of_turn, build_sq)) {
                    actions.push_back(BUILD_2_OFFSET + build_sq);
                }
            }
            std::sort(actions.begin(), actions.end());
            actions.erase(std::unique(actions.begin(), actions.end()), actions.end());
        }
        else if (god == Constants::God::DEMETER) {
            actions.push_back(PASS_ACTION);
            for (sq_i build_sq : Constants::NEIGHBOURS[_active_worker]) {
                if (build_sq != _last_build_sq && _build_ok(_start_sq_of_turn, _active_worker, build_sq)) {
                    actions.push_back(BUILD_2_OFFSET + build_sq);
                }
            }
        }
        else if (god == Constants::God::HEPHAESTUS) {
            actions.push_back(PASS_ACTION);
            if (_blocks[_last_build_sq] < 3) { // Cannot dome on second build
                actions.push_back(BUILD_2_OFFSET + _last_build_sq);
            }
        }
        else if (god == Constants::God::ATLAS) {
            actions.push_back(PASS_ACTION);
            if (_blocks[_last_build_sq] < 4) {
                actions.push_back(DOME_ACTION);
            }
        }
    }

    return actions;
}

int Board::check_state() {
    if (_won) return _turn; // The current player is the winner

    // If a player reaches a phase and has absolutely no legal actions
    // (e.g. trapped in Phase 0, or moved into a corner and can't build in Phase 1)
    if (generate_legal_actions().empty()) {
        return -_turn; // The current player loses
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

    if (_turn == -1) h ^= Constants::ZOBRIST_TURN;
    if (_prevent_up_next_turn) h ^= Constants::ATHENA_EFFECT;

    // Sub-turn specific hashing to prevent transposition table collisions
    h ^= (static_cast<uint64_t>(_current_phase) * 0x123456789ABCDEFULL);
    if (_active_worker != -1) h ^= (static_cast<uint64_t>(_active_worker) * 0xFEDCBA987654321ULL);
    if (_prometheus_built_early) h ^= 0x9999999999999999ULL;

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

std::optional<int> Board::_which_worker_is_here(sq_i s) const {
    auto oc = _workers_map[s];
    if (oc == -1) return std::nullopt;
    return oc;
}

bool Board::is_free(sq_i s) const {
    if (_blocks[s] >= 4) return false;
    return _workers_map[s] == -1;
}

void Board::_move_worker(sq_i from, sq_i to) {
    auto idx = _workers_map[from];
    _workers[idx] = to;
    if (from == to) return;
    _workers_map[to] = idx;
    _workers_map[from] = -1;
}

void Board::_swap_workers(sq_i sq1, sq_i sq2) {
    int w1_idx = *_which_worker_is_here(sq1);
    int w2_idx = *_which_worker_is_here(sq2);
    _workers[w1_idx] = sq2;
    _workers[w2_idx] = sq1;
    _workers_map[sq1] = w2_idx;
    _workers_map[sq2] = w1_idx;
}

void Board::_inc_block(sq_i s) {
    _blocks[s]++;
}

void Board::_dec_block(sq_i s) {
    _blocks[s]--;
}

void Board::_restore_block_height(sq_i s, int8_t original_height) {
    _blocks[s] = original_height;
}

bool adj_ok(sq_i from, sq_i to) {
    return Constants::ADJACENCY_MATRIX[from][to];
}

std::optional<sq_i> Board::_calculate_push_square(sq_i from_sq, sq_i to_sq) const {
    int dx = (to_sq % 5) - (from_sq % 5);
    int dy = (to_sq / 5) - (from_sq / 5);
    int push_r = (to_sq / 5) + dy;
    int push_c = (to_sq % 5) + dx;
    if (push_r >= 0 && push_r <= 4 && push_c >= 0 && push_c <= 4) {
        return static_cast<sq_i>(push_r * 5 + push_c);
    }
    return std::nullopt;
}

bool Board::_height_ok(sq_i from, sq_i to) const {
    return _blocks[to] - _blocks[from] <= 1;
}

bool Board::move_checks(sq_i from, sq_i to) const {
    return _height_ok(from, to) && adj_ok(from, to) && is_free(to);
}

bool Board::_build_ok(sq_i old_sq, sq_i curr_sq, sq_i build) const {
    if (!adj_ok(curr_sq, build) || curr_sq == build) return false;
    return (old_sq == build) || is_free(build);
}

bool Board::_complete_checks(sq_i from, sq_i to, sq_i build) const {
    return move_checks(from, to) && _build_ok(from, to, build);
}

bool Board::_blocked_by_athena(const int from_sq, const int to_sq) const {
    return _prevent_up_next_turn && _blocks[to_sq] > _blocks[from_sq];
}

bool operator<(const Board &lhs, const Board &rhs) {
    return lhs._hash < rhs._hash;
}

} // namespace Santorini