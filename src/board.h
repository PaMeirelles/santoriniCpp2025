#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <memory>

#include "constants.h"
#include "moves.h"

namespace Santorini {

class Board {
public:
    // --- Public Interface ---
    explicit Board(const std::string& position);
    bool move_is_valid(const Moves::Move& move) const;
    void make_move(const Moves::Move& move);
    void unmake_move(const Moves::Move& move);
    void make_null_move();
    void unmake_null_move(bool prev_prevent_up_flag);
    std::vector<std::unique_ptr<Moves::Move>> generate_moves() const;
    int check_state();
    std::string to_text() const;
    uint64_t get_hash() const { return _hash; }

    // --- Getters for Evaluation ---
    const std::array<sq_i, 4>& get_workers() const { return _workers; }
    const std::array<sq_i, 25>& get_blocks() const { return _blocks; }
    int8_t get_turn() const { return _turn; }
    bool is_free(sq_i s) const;
    const std::array<Constants::God, 2>& get_gods() const { return _gods; }
    bool get_prevent_up_next_turn() const { return _prevent_up_next_turn; }

private:
    // --- Board State ---
    std::array<sq_i, 25> _blocks{};
    std::array<sq_i, 4> _workers{};
    int8_t _turn = 1;
    std::array<Constants::God, 2> _gods{};
    bool _prevent_up_next_turn = false;
    int _last_move_height_diff = 0;
    bool _won = false;
    uint64_t _hash = 0;

    // --- Private Helper Methods ---
    void _parse_position(const std::string& position);
    uint64_t _calculate_full_hash() const;
    void _xor_hash(uint64_t value) { _hash ^= value; }
    int _player_of_worker(int worker_index) const;
    bool _is_opponent_worker(int worker_index) const;
    bool _is_ally_worker(int worker_index) const;
    std::optional<int> _which_worker_is_here(sq_i s) const;
    void _move_worker(sq_i from,sq_i to);
    void _move_worker_back(int worker_idx,sq_i original_pos);
    void _inc_block(sq_i s);
    void _dec_block(sq_i s);
    void _restore_block_height(sq_i s, int8_t original_height);
    bool _height_ok(sq_i from,sq_i to) const;
    bool _adj_ok(sq_i from,sq_i to) const;
    std::optional<sq_i> _calculate_push_square(sq_i from_sq,sq_i to_sq);
    bool _move_checks(sq_i from,sq_i to) const;
    bool _build_ok(sq_i from, sq_i to, sq_i build) const;
    bool _complete_checks(sq_i from, sq_i to, sq_i build) const;
    bool _player_has_any_valid_move();

    // --- God-Specific Logic ---
    void _execute_god_move(const Moves::Move& move);
    void _undo_god_move(const Moves::Move& move);
    bool _validate_god_move(const Moves::Move& move) const;
    std::vector<std::unique_ptr<Moves::Move>> _generate_god_moves() const;

    // ... Declarations for each god's validation, execution, undo, and generation functions
    bool _is_valid_apollo(const Moves::ApolloMove& move) const;
    void _execute_apollo(const Moves::ApolloMove& move);
    void _undo_apollo(const Moves::ApolloMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_apollo_moves() const;

    bool _is_valid_artemis(const Moves::ArtemisMove& move) const;
    void _execute_artemis(const Moves::ArtemisMove& move);
    void _undo_artemis(const Moves::ArtemisMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_artemis_moves() const;

    bool _is_valid_athena(const Moves::AthenaMove& move) const;
    void _execute_athena(const Moves::AthenaMove& move);
    void _undo_athena(const Moves::AthenaMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_athena_moves() const;

    bool _is_valid_atlas(const Moves::AtlasMove& move) const;
    void _execute_atlas(const Moves::AtlasMove& move);
    void _undo_atlas(const Moves::AtlasMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_atlas_moves() const;

    bool _is_valid_demeter(const Moves::DemeterMove& move) const;
    void _execute_demeter(const Moves::DemeterMove& move);
    void _undo_demeter(const Moves::DemeterMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_demeter_moves() const;

    bool _is_valid_hephaestus(const Moves::HephaestusMove& move) const;
    void _execute_hephaestus(const Moves::HephaestusMove& move);
    void _undo_hephaestus(const Moves::HephaestusMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_hephaestus_moves() const;

    bool _is_valid_hermes(const Moves::HermesMove& move) const;
    void _execute_hermes(const Moves::HermesMove& move);
    void _undo_hermes(const Moves::HermesMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_hermes_moves() const;

    bool _is_valid_minotaur(const Moves::MinotaurMove& move) const;
    void _execute_minotaur(const Moves::MinotaurMove& move);
    void _undo_minotaur(const Moves::MinotaurMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_minotaur_moves() const;

    bool _is_valid_pan(const Moves::PanMove& move) const;
    void _execute_pan(const Moves::PanMove& move);
    void _undo_pan(const Moves::PanMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_pan_moves() const;

    bool _is_valid_prometheus(const Moves::PrometheusMove& move) const;
    void _execute_prometheus(const Moves::PrometheusMove& move);
    void _undo_prometheus(const Moves::PrometheusMove& move);
    std::vector<std::unique_ptr<Moves::Move>> _generate_prometheus_moves() const;

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
    friend bool operator<(const Board& lhs, const Board& rhs);
};
    bool operator<(const Board& lhs, const Board& rhs);
} // namespace Santorini
