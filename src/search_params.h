#pragma once

namespace Santorini {

    /**
     * @struct SearchParams
     * @brief Holds all tunable parameters for the search algorithm.
     *
     * This struct centralizes the configuration of the search, allowing for
     * easy modification and tuning without changing the core search code.
     */
    struct SearchParams {
        // Aspiration Windows
        int asp_window = 50;

        // Late Move Reduction (LMR)
        int min_depth = 2;
        int n_protected_moves = 2;
        float lmr_factor = 4.0f;

        // Reverse Futility Pruning (RFP)
        int rfp_depth = 4;
        int rfp_margin = 250;

        // Futility Pruning
        int futility_pruning_max_depth = 6;
        int futility_base_margin = -50;
        int futility_depth_margin = 250;

        // Razoring
        int razor_depth = 4;
        int razor_margin = 125;

        // NMP
        float nmp_factor = 1.0f;
    };

} // namespace Santorini