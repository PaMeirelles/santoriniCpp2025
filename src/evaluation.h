#pragma once

#include <vector>
#include <array>
#include <algorithm> // Required for std::count
#include <cmath>     // Required for std::pow
#include "board.h"
#include "constants.h"

namespace Santorini {

// --- Power Score Helper Function (Unchanged) ---
inline double power_score(const int count, const double multiplier, const double power) {
    if (count == 0 || multiplier == 0.0) {
        return 0.0;
    }
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

constexpr int TEMPO = 60;

struct Parameters {
    std::vector<int> posScore;
    std::vector<int> heightScore;

    // Height-specific support parameters
    double sh0_mult, sh0_power, nh0_mult, nh0_power, nn0_mult, nn0_power; // h=0
    double sh1_mult, sh1_power, nh1_mult, nh1_power, ph1_mult, ph1_power, nn1_mult, nn1_power; // h=1
    double sh2_mult, sh2_power, nh2_mult, nh2_power, ph2_mult, ph2_power; // h=2
    double nh2_global_mult, nh2_global_power;

    Parameters(int centrality_gap, int h2_gap,
               // Height 0 params
               double sh0_m, double sh0_p, double nh0_m, double nh0_p, double nn0_m, double nn0_p,
               // Height 1 params
               double sh1_m, double sh1_p, double nh1_m, double nh1_p, double ph1_m, double ph1_p, double nn1_m, double nn1_p,
               // Height 2 params
               double sh2_m, double sh2_p, double nh2_m, double nh2_p, double ph2_m, double ph2_p,
               // Global params
               double nh2_global_m, double nh2_global_p)
        : sh0_mult(sh0_m), sh0_power(sh0_p), nh0_mult(nh0_m), nh0_power(nh0_p), nn0_mult(nn0_m), nn0_power(nn0_p),
          sh1_mult(sh1_m), sh1_power(sh1_p), nh1_mult(nh1_m), nh1_power(nh1_p), ph1_mult(ph1_m), ph1_power(ph1_p), nn1_mult(nn1_m), nn1_power(nn1_p),
          sh2_mult(sh2_m), sh2_power(sh2_p), nh2_mult(nh2_m), nh2_power(nh2_p), ph2_mult(ph2_m), ph2_power(ph2_p),
          nh2_global_mult(nh2_global_m), nh2_global_power(nh2_global_p) {

        posScore.resize(25);
        for (int i = 0; i < 25; ++i) {
            posScore[i] = centrality_gap * POS_GAPS[i];
        }
        heightScore = {0, 100, h2_gap + 100, h2_gap + 50};
    }
};

const Parameters PARAMS(
    /*centrality_gap=*/45,
    /*h2_gap=*/475,
    // Height 0
    /*sh0_mult=*/-20,   /*sh0_power=*/0.5500,
    /*nh0_mult=*/60,    /*nh0_power=*/0.6500,
    /*nn0_mult=*/50,    /*nn0_power=*/1.1500,
    // Height 1
    /*sh1_mult=*/55,    /*sh1_power=*/0.2300,
    /*nh1_mult=*/185,   /*nh1_power=*/0.7500,
    /*ph1_mult=*/-5,    /*ph1_power=*/0.8000,
    /*nn1_mult=*/20,    /*nn1_power=*/-0.2500,
    // Height 2
    /*sh2_mult=*/220,   /*sh2_power=*/0.5000,
    /*nh2_mult=*/480,   /*nh2_power=*/0.9000,
    /*ph2_mult=*/20,    /*ph2_power=*/-0.3500,
    // Global
    /*nh2_global_mult=*/-305, /*nh2_global_power=*/0.1500
);

inline int score_position(const Board& b, const Parameters& params = PARAMS) {
    // Count all towers of height 3 on the board once.
    const auto& blocks = b.get_blocks();
    const int h3_towers_count = std::count(blocks.begin(), blocks.end(), 3);

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

        // Calculate support score using height-specific parameters
        double support = 0.0;
        if (height == 0) {
            support += power_score(same_h, params.sh0_mult, params.sh0_power);
            support += power_score(next_h, params.nh0_mult, params.nh0_power);
            support += power_score(next_next_h, params.nn0_mult, params.nn0_power);
        } else if (height == 1) {
            support += power_score(same_h, params.sh1_mult, params.sh1_power);
            support += power_score(next_h, params.nh1_mult, params.nh1_power);
            support += power_score(prev_h, params.ph1_mult, params.ph1_power);
            support += power_score(next_next_h, params.nn1_mult, params.nn1_power);
        } else if (height == 2) {
            support += power_score(same_h, params.sh2_mult, params.sh2_power);
            support += power_score(next_h, params.nh2_mult, params.nh2_power);
            support += power_score(prev_h, params.ph2_mult, params.ph2_power);
            support += power_score(h3_towers_count, params.nh2_global_mult, params.nh2_global_power);
        }

        return p_score + h_score + static_cast<int>(support);
    };

    const int p1_score = score_worker(0) + score_worker(1);
    const int p2_score = score_worker(2) + score_worker(3);

    const int tempo_score = (b.get_turn() == 1) ? TEMPO : -TEMPO;

    return p1_score - p2_score + tempo_score;
}

} // namespace Santorini