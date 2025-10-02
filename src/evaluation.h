#pragma once

#include <vector>
#include <algorithm>
#include <cmath> // Required for std::pow
#include "board.h"
#include "constants.h"

namespace Santorini {

// --- New Power Score Helper Function ---
// Replaces the previous log_score function.
inline double power_score(const int count, const double multiplier, const double power) {
    if (count == 0 || multiplier == 0.0) {
        return 0.0;
    }
    // Calculates score based on the model: y = multiplier * count^power
    return multiplier * std::pow(count, power);
}

// --- Constants (Unchanged) ---
const std::array POS_GAPS = {
    0, 1, 2, 1, 0,
    1, 2, 3, 2, 1,
    2, 3, 4, 3, 2,
    1, 2, 3, 2, 1,
    0, 1, 2, 1, 0
};

constexpr int TEMPO = 50;

// --- Parameters Struct (Refactored for Power Function) ---
struct Parameters {
    std::vector<int> posScore;
    std::vector<int> heightScore;

    // Support parameters are now multipliers and powers
    double sh_mult, sh_power;
    double nh_mult, nh_power;
    double ph_mult, ph_power;
    double nn_mult, nn_power;

    Parameters(int centrality_gap, int h2_gap,
               double sh_mult, double sh_power, double nh_mult, double nh_power,
               double ph_mult, double ph_power, double nn_mult, double nn_power)
        : sh_mult(sh_mult), sh_power(sh_power),
          nh_mult(nh_mult), nh_power(nh_power),
          ph_mult(ph_mult), ph_power(ph_power),
          nn_mult(nn_mult), nn_power(nn_power) {

        posScore.resize(25);
        for (int i = 0; i < 25; ++i) {
            posScore[i] = centrality_gap * POS_GAPS[i];
        }
        heightScore = {0, 100, h2_gap + 100, h2_gap + 50};
    }
};

// --- Global Parameters Instance (Updated for Power Function) ---
// These values were calculated to approximate the previous log model's behavior.
// The multiplier is the score for the 1st neighbor, and the power controls
// the rate of diminishing returns.
const Parameters PARAMS(
    /*centrality_gap=*/25,
    /*h2_gap=*/250,
    /*sh_mult=*/60, /*sh_power=*/0.3691,
    /*nh_mult=*/80, /*nh_power=*/0.3691,
    /*ph_mult=*/20, /*ph_power=*/0.3691,
    /*nn_mult=*/25, /*nn_power=*/0.4278
);

// --- Main Evaluation Function (Updated) ---
inline int score_position(const Board& b, const Parameters& params = PARAMS) {
    auto score_worker = [&](const int worker_idx) -> int {
        const sq_i square = b.get_workers()[worker_idx];
        const int height = b.get_blocks()[square];

        const int p_score = params.posScore[square];
        const int h_score = params.heightScore[height];

        int same_h = 0, next_h = 0, next_next_h = 0, prev_h = 0;

        for (sq_i n : Constants::NEIGHBOURS[square]) {
            if (b.is_free(n)) {
                int h = b.get_blocks()[n];
                if (h == height) same_h++;
                else if (h == height + 1) next_h++;
                else if (h == height - 1) prev_h++;
                else if (h == height + 2) next_next_h++;
            }
        }

        if (height == 0) {
            prev_h = 0;
            same_h = 0;
        }
        if (height >= 2) {
            next_next_h = 0;
        }

        // The support score is now calculated using the new power_score function
        const double support = power_score(same_h, params.sh_mult, params.sh_power)
                             + power_score(next_h, params.nh_mult, params.nh_power)
                             + power_score(prev_h, params.ph_mult, params.ph_power)
                             + power_score(next_next_h, params.nn_mult, params.nn_power);

        return p_score + h_score + static_cast<int>(support);
    };

    const int p1_score = score_worker(0) + score_worker(1);
    const int p2_score = score_worker(2) + score_worker(3);

    const int tempo_score = (b.get_turn() == 1) ? TEMPO : 0;

    return p1_score - p2_score + tempo_score;
}

} // namespace Santorini