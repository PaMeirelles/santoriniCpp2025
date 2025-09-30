#pragma once

#include <vector>
#include <algorithm>
#include "board.h"
#include "constants.h"

namespace Santorini {

const std::array POS_GAPS = {
    0, 1, 2, 1, 0,
    1, 2, 3, 2, 1,
    2, 3, 4, 3, 2,
    1, 2, 3, 2, 1,
    0, 1, 2, 1, 0
};

struct Parameters {
    std::vector<int> posScore;
    std::vector<int> heightScore;
    std::vector<int> sameHeightSupport;
    std::vector<int> nextHeightSupport;
    std::vector<int> prevHeightSupport;
    std::vector<int> nextNextHeightSupport;

    Parameters(int centrality_gap, int h2_gap, int sh1, int sh2, int nh1, int nh2, int ph1, int ph2, int nn1, int nn2) {
        posScore.resize(25);
        for (int i = 0; i < 25; ++i) {
            posScore[i] = centrality_gap * POS_GAPS[i];
        }
        heightScore = {0, 100, h2_gap + 100, h2_gap + 50};
        sameHeightSupport = {0, sh1, sh1 + sh2};
        nextHeightSupport = {0, nh1, nh1 + nh2};
        prevHeightSupport = {0, ph2, ph1 + ph2};
        nextNextHeightSupport = {0, nn2, nn1 + nn2};
    }
};

const Parameters PARAMS(
    /*centrality_gap=*/23,
    /*h2_gap=*/235,
    /*sh1=*/48, /*sh2=*/17,
    /*nh1=*/89, /*nh2=*/43,
    /*ph1=*/16, /*ph2=*/0,
    /*nn1=*/13, /*nn2=*/31
);

constexpr int TEMPO = 50;

inline int score_position(const Board& b, const Parameters& params = PARAMS) {
    auto score_worker = [&](const int worker_idx) -> int {
        const sq_i square = b.get_workers()[worker_idx];
        const int height = b.get_blocks()[square];

        const int p_score = params.posScore[square];
        const int h_score = params.heightScore[height];

        int same_h = 0;
        int next_h = 0;
        int next_next_h = 0;
        int prev_h = 0;

        for (sq_i n : Constants::NEIGHBOURS[square]) {
            if (b.is_free(n)) {
                int h = b.get_blocks()[n];
                if (h == height) same_h++;
                else if (h == height + 1) next_h++;
                else if (h == height - 1) prev_h++;
                else if (h == height + 2) next_next_h++;
            }
        }

        same_h = std::min(same_h, 2);
        next_h = std::min(next_h, 2);

        if (height != 0) {
            if (same_h == 0) prev_h = std::min(prev_h, 2);
            else if (same_h == 1) prev_h = (prev_h >= 1) ? 1 : 0;
            else prev_h = 0;
        } else {
            prev_h = 0;
            same_h = 0;
        }

        if (height < 2) {
            if (next_h == 0) next_next_h = std::min(next_next_h, 2);
            else if (next_h == 1) next_next_h = (next_next_h >= 1) ? 1 : 0;
            else next_next_h = 0;
        } else {
            next_next_h = 0;
        }

        const int support = params.sameHeightSupport[same_h]
                    + params.nextHeightSupport[next_h]
                    + params.prevHeightSupport[prev_h]
                    + params.nextNextHeightSupport[next_next_h];

        return p_score + h_score + support;
    };

    int tempo_score = (b.get_turn() == 1) ? TEMPO : 0;
    return score_worker(0) + score_worker(1) - score_worker(2) - score_worker(3) + tempo_score;
}

} // namespace Santorini
