#pragma once

#include <vector>
#include <algorithm>
#include <cmath> // Required for std::log
#include "board.h"
#include "constants.h"

namespace Santorini {


inline double log_score(const int count, const double multiplier, const double base) {
    if (count == 0 || base <= 1.0 || multiplier == 0.0) {
        return 0.0;
    }
    // Using change of base formula: log_base(x) = log(x) / log(base)
    // We use log(count + 1) so a count of 1 gives a positive score.
    return multiplier * (std::log(count + 1) / std::log(base));
}

const std::array POS_GAPS = {
    0, 1, 2, 1, 0,
    1, 2, 3, 2, 1,
    2, 3, 4, 3, 2,
    1, 2, 3, 2, 1,
    0, 1, 2, 1, 0
};

constexpr int TEMPO = 50;
// Isolation bonus is set to 0 to match the tuning script configuration.

struct Parameters {
    std::vector<int> posScore;
    std::vector<int> heightScore;

    // Support parameters are now multipliers and bases for the log function
    double sh_mult, sh_base;
    double nh_mult, nh_base;
    double ph_mult, ph_base;
    double nn_mult, nn_base;

    Parameters(int centrality_gap, int h2_gap,
               double sh_mult, double sh_base, double nh_mult, double nh_base,
               double ph_mult, double ph_base, double nn_mult, double nn_base)
        : sh_mult(sh_mult), sh_base(sh_base),
          nh_mult(nh_mult), nh_base(nh_base),
          ph_mult(ph_mult), ph_base(ph_base),
          nn_mult(nn_mult), nn_base(nn_base) {

        posScore.resize(25);
        for (int i = 0; i < 25; ++i) {
            posScore[i] = centrality_gap * POS_GAPS[i];
        }
        heightScore = {0, 100, h2_gap + 100, h2_gap + 50};
    }
};

const Parameters PARAMS(
    /*centrality_gap=*/43,
    /*h2_gap=*/425,
    /*sh_mult=*/80.0, /*sh_base=*/2.2,
    /*nh_mult=*/114.0, /*nh_base=*/2.0,
    /*ph_mult=*/22.0, /*ph_base=*/1.5,
    /*nn_mult=*/35.0, /*nn_base=*/1.7
);


// --- Main Evaluation Function (Refactored) ---

inline int score_position(const Board& b, const Parameters& params = PARAMS) {
    auto score_worker = [&](const int worker_idx) -> int {
        const sq_i square = b.get_workers()[worker_idx];
        const int height = b.get_blocks()[square];

        const int p_score = params.posScore[square];
        const int h_score = params.heightScore[height];

        int same_h = 0, next_h = 0, next_next_h = 0, prev_h = 0;

        // Neighbor counting logic remains the same
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

        const double support = log_score(same_h, params.sh_mult, params.sh_base)
                             + log_score(next_h, params.nh_mult, params.nh_base)
                             + log_score(prev_h, params.ph_mult, params.ph_base)
                             + log_score(next_next_h, params.nn_mult, params.nn_base);

        return p_score + h_score + static_cast<int>(support);
    };

    const int p1_score = score_worker(0) + score_worker(1);
    const int p2_score = score_worker(2) + score_worker(3);

    const int tempo_score = (b.get_turn() == 1) ? TEMPO : 0;

    return p1_score - p2_score + tempo_score;
}

} // namespace Santorini