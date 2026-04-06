#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <memory>
#include "constants.h"

// If you have completely migrated away from the Move struct in your test suite,
// you can safely delete the moves.h file entirely.
// #include "moves.h"

namespace Santorini {

class Board {
public:

    // --- Action Space Constants ---
    static constexpr int MOVE_OFFSET = 0;
    static constexpr int BUILD_1_OFFSET = 625;
    static constexpr int BUILD_2_OFFSET = 650;
    static constexpr int PASS_ACTION = 675;
    static constexpr int DOME_ACTION = 676;
    static constexpr int TOTAL_ACTIONS = 677;

    // --- Public Interface ---
    explicit Board(const std::string& position);

    void make_action(int action_idx);
    std::vector<int> generate_legal_actions() const;
    int check_state();

    std::string to_text() const;
    bool _sanity_check_workers() const;
    uint64_t get_hash() const { return _hash; }

    // --- Getters for Evaluation ---
    const std::array<sq_i, 4>& get_workers() const { return _workers; }
    const std::array<int, 25>& get_workers_map() const { return _workers_map; }
    const std::array<sq_i, 25>& get_blocks() const { return _blocks; }
    int8_t get_turn() const { return _turn; }
    bool is_free(sq_i s) const;
    const std::array<Constants::God, 2>& get_gods() const { return _gods; }
    bool get_prevent_up_next_turn() const { return _prevent_up_next_turn; }
    Constants::God get_current_god() const { return _turn == 1 ? _gods[0] : _gods[1]; }
    int get_current_phase() const { return _current_phase; }
    bool get_won() const { return _won; }
    bool move_checks(sq_i from, sq_i to) const;

private:

    // --- Board State ---
    std::array<sq_i, 25> _blocks{};
    std::array<sq_i, 4> _workers{};
    std::array<int, 25> _workers_map{};
    int8_t _turn = 1;
    std::array<Constants::God, 2> _gods{};
    bool _prevent_up_next_turn = false;
    int _last_move_height_diff = 0;
    bool _won = false;
    uint64_t _hash = 0;

    // --- State Machine Trackers ---
    int _current_phase = 0;     // 0: Move, 1: Build 1, 2: Build 2
    int _active_worker = -1;
    int _last_build_sq = -1;
    int _start_sq_of_turn = -1;
    bool _prometheus_built_early = false;

    // --- Private Helper Methods ---
    void _parse_position(const std::string& position);
    uint64_t _calculate_full_hash() const;
    void _xor_hash(uint64_t value) { _hash ^= value; }

    int _player_of_worker(int worker_index) const;
    bool _is_opponent_worker(int worker_index) const;
    bool _is_ally_worker(int worker_index) const;
    std::optional<int> _which_worker_is_here(sq_i s) const;

    void _move_worker(sq_i from, sq_i to);
    void _swap_workers(sq_i sq1, sq_i sq2);
    void _inc_block(sq_i s);
    void _dec_block(sq_i s);
    void _restore_block_height(sq_i s, int8_t original_height);

    bool _height_ok(sq_i from, sq_i to) const;
    std::optional<sq_i> _calculate_push_square(sq_i from_sq, sq_i to_sq) const;
    bool _build_ok(sq_i from, sq_i to, sq_i build) const;
    bool _complete_checks(sq_i from, sq_i to, sq_i build) const;
    bool _blocked_by_athena(int from_sq, int to_sq) const;

    // Ends the full turn, resetting phase to 0, toggling the turn, and applying Athena's logic
    void _end_turn();


    // --- Test Framework Friends ---
    // Note: If your tests previously relied on unmake_move() or generate_moves(),
    // you will need to update them to simulate sub-turn integer actions.
    friend class ApolloTests_swap_up_one_height_Test;
    friend class ApolloTests_can_only_swap_with_enemy_Test;
    friend class ApolloTests_no_moves_but_apollo_swap_saves_you_Test;
    friend class ApolloTests_no_build_on_from_when_swapping_Test;
    friend class ApolloTests_no_swap_can_build_Test;
    friend class ApolloTests_misc_Test;
    friend class AthenaTests_athena_power_works_Test;
    friend class AthenaTests_opponent_generated_moves_do_not_climb_after_athena_up_Test;
    friend class AtlasTests_no_one_moves_on_domes_Test;
    friend class DemeterTests_cannot_build_twice_on_same_square_Test;
    friend class DemeterTests_can_build_only_once_if_desired_Test;
    friend class TestsHephaestus_cannot_build_twice_on_different_squares_Test;
    friend class TestsHephaestus_can_build_only_once_if_desired_Test;
    friend class TestsHephaestus_cannot_dome_on_second_build_Test;
    friend class TestsHephaestus_can_dome_normally_Test;
    friend class TestsHephaestus_misc_Test;
    friend class TestsPrometheus_can_move_normally_if_you_want_Test;
    friend class TestsPrometheus_second_build_must_be_before_moving_Test;
    friend class TestsPrometheus_cannot_move_up_if_built_before_moving_Test;
    friend class TestsPrometheus_cannot_move_up_to_newly_built_Test;
    friend class TestBoardHashing_BlockHashingOk_Test;
    friend class ClimberQuietMoveTests_VerifyPropertiesAndCompleteness_Test;
    friend bool operator<(const Board& lhs, const Board& rhs);
};

bool operator<(const Board& lhs, const Board& rhs);
bool adj_ok(sq_i from, sq_i to);

} // namespace Santorini