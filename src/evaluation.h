#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <cmath> // Required for std::pow
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

constexpr int TEMPO = 50;

// --- Parameters Struct (Unchanged) ---
struct Parameters {
    std::vector<int> posScore;
    std::vector<int> heightScore;

    // Height-specific support parameters
    double sh0_mult, sh0_power, nh0_mult, nh0_power, nn0_mult, nn0_power; // h=0
    double sh1_mult, sh1_power, nh1_mult, nh1_power, ph1_mult, ph1_power, nn1_mult, nn1_power; // h=1
    double sh2_mult, sh2_power, nh2_mult, nh2_power, ph2_mult, ph2_power; // h=2

    Parameters(int centrality_gap, int h2_gap,
               // Height 0 params
               double sh0_m, double sh0_p, double nh0_m, double nh0_p, double nn0_m, double nn0_p,
               // Height 1 params
               double sh1_m, double sh1_p, double nh1_m, double nh1_p, double ph1_m, double ph1_p, double nn1_m, double nn1_p,
               // Height 2 params
               double sh2_m, double sh2_p, double nh2_m, double nh2_p, double ph2_m, double ph2_p)
        : sh0_mult(sh0_m), sh0_power(sh0_p), nh0_mult(nh0_m), nh0_power(nh0_p), nn0_mult(nn0_m), nn0_power(nn0_p),
          sh1_mult(sh1_m), sh1_power(sh1_p), nh1_mult(nh1_m), nh1_power(nh1_p), ph1_mult(ph1_m), ph1_power(ph1_p), nn1_mult(nn1_m), nn1_power(nn1_p),
          sh2_mult(sh2_m), sh2_power(sh2_p), nh2_mult(nh2_m), nh2_power(nh2_p), ph2_mult(ph2_m), ph2_power(ph2_p) {

        posScore.resize(25);
        for (int i = 0; i < 25; ++i) {
            posScore[i] = centrality_gap * POS_GAPS[i];
        }
        heightScore = {0, 100, h2_gap + 100, h2_gap + 50};
    }
};

// --- Global Parameters Instance (UPDATED to match evaluation.py) ---
const Parameters PARAMS(
    /*centrality_gap=*/50,      //
    /*h2_gap=*/375,            //
    // Height 0
    /*sh0_mult=*/30.0,         //
    /*sh0_power=*/-0.2,        //
    /*nh0_mult=*/45.0,         //
    /*nh0_power=*/0.8,         //
    /*nn0_mult=*/55.0,         //
    /*nn0_power=*/1.15,        //
    // Height 1
    /*sh1_mult=*/50.0,         //
    /*sh1_power=*/0.3,         //
    /*nh1_mult=*/180.0,        //
    /*nh1_power=*/0.75,        //
    /*ph1_mult=*/55.0,         //
    /*ph1_power=*/-0.35,       //
    /*nn1_mult=*/55.0,         //
    /*nn1_power=*/0.05,        //
    // Height 2
    /*sh2_mult=*/210.0,        //
    /*sh2_power=*/0.55,        //
    /*nh2_mult=*/445.0,        //
    /*nh2_power=*/1.15,        //
    /*ph2_mult=*/-15.0,        //
    /*ph2_power=*/0.95         //
);

using SupportTable = std::array<double, 9>;

inline SupportTable make_support_table(double multiplier, double exponent) {
    SupportTable table{};
    for (int count = 0; count < static_cast<int>(table.size()); ++count) {
        table[count] = power_score(count, multiplier, exponent);
    }
    return table;
}

const struct {
    SupportTable same_height_0, next_height_0, two_above_0;
    SupportTable same_height_1, next_height_1, below_1, two_above_1;
    SupportTable same_height_2, next_height_2, below_2;
} DEFAULT_SUPPORT = {
    make_support_table(PARAMS.sh0_mult, PARAMS.sh0_power),
    make_support_table(PARAMS.nh0_mult, PARAMS.nh0_power),
    make_support_table(PARAMS.nn0_mult, PARAMS.nn0_power),
    make_support_table(PARAMS.sh1_mult, PARAMS.sh1_power),
    make_support_table(PARAMS.nh1_mult, PARAMS.nh1_power),
    make_support_table(PARAMS.ph1_mult, PARAMS.ph1_power),
    make_support_table(PARAMS.nn1_mult, PARAMS.nn1_power),
    make_support_table(PARAMS.sh2_mult, PARAMS.sh2_power),
    make_support_table(PARAMS.nh2_mult, PARAMS.nh2_power),
    make_support_table(PARAMS.ph2_mult, PARAMS.ph2_power),
};

// Custom parameter sets keep the live calculation so callers can change their weights.
template <bool UseDefaultTable>
inline int score_position_impl(const Board& b, const Parameters& params) {
    const auto support_value = [](int count, const SupportTable& table, double multiplier, double exponent) {
        if constexpr (UseDefaultTable) {
            return table[count];
        } else {
            return power_score(count, multiplier, exponent);
        }
    };
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
            support += support_value(same_h, DEFAULT_SUPPORT.same_height_0, params.sh0_mult, params.sh0_power);
            support += support_value(next_h, DEFAULT_SUPPORT.next_height_0, params.nh0_mult, params.nh0_power);
            support += support_value(next_next_h, DEFAULT_SUPPORT.two_above_0, params.nn0_mult, params.nn0_power);
        } else if (height == 1) {
            support += support_value(same_h, DEFAULT_SUPPORT.same_height_1, params.sh1_mult, params.sh1_power);
            support += support_value(next_h, DEFAULT_SUPPORT.next_height_1, params.nh1_mult, params.nh1_power);
            support += support_value(prev_h, DEFAULT_SUPPORT.below_1, params.ph1_mult, params.ph1_power);
            support += support_value(next_next_h, DEFAULT_SUPPORT.two_above_1, params.nn1_mult, params.nn1_power);
        } else if (height == 2) {
            support += support_value(same_h, DEFAULT_SUPPORT.same_height_2, params.sh2_mult, params.sh2_power);
            support += support_value(next_h, DEFAULT_SUPPORT.next_height_2, params.nh2_mult, params.nh2_power);
            support += support_value(prev_h, DEFAULT_SUPPORT.below_2, params.ph2_mult, params.ph2_power);
        }

        return p_score + h_score + static_cast<int>(support);
    };

    const int p1_score = score_worker(0) + score_worker(1);
    const int p2_score = score_worker(2) + score_worker(3);

    const int tempo_score = (b.get_turn() == 1) ? TEMPO : 0;

    return p1_score - p2_score + tempo_score;
}

inline int score_position(const Board& b, const Parameters& params = PARAMS) {
    if (&params == &PARAMS) {
        return score_position_impl<true>(b, params);
    }
    return score_position_impl<false>(b, params);
}

} // namespace Santorini