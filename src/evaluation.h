#pragma once

#include <vector>
#include <array>
#include <iostream>
#include <filesystem>
#include <torch/script.h>
#include "board.h"
#include "constants.h"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace Santorini {

    namespace fs = std::filesystem;

    /**
     * Helper to find the directory where the current .exe is located.
     */
    inline fs::path get_executable_dir() {
#ifdef _WIN32
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return fs::path(path).parent_path();
#else
        // Fallback for Linux if you ever move there
        return fs::read_symlink("/proc/self/exe").parent_path();
#endif
    }

    inline torch::jit::script::Module& get_evaluator_model() {
        static torch::jit::script::Module module;
        static bool is_loaded = false;

        if (!is_loaded) {
            try {
                // Get the absolute path to the model sitting next to the EXE
                fs::path model_path = get_executable_dir() / "santorini_evaluator.pt";

                if (!fs::exists(model_path)) {
                    std::cerr << "[ERROR] Model file not found at: " << model_path.string() << "\n";
                    std::cerr << "Make sure 'santorini_evaluator.pt' is in the same folder as the .exe\n";
                    std::exit(-1);
                }

                // Load from the absolute path string
                module = torch::jit::load(model_path.string());

                module.to(torch::kCUDA);

                module.eval();
                is_loaded = true;

            } catch (const c10::Error& e) {
                std::cerr << "LibTorch error: " << e.msg() << "\n";
                std::exit(-1);
            }
        }
        return module;
    }

inline int score_position(const Board& b) {
    // 1. Construct CPU tensor first (Faster to index on CPU before moving to GPU)
    torch::Tensor cpu_tensor = torch::zeros({1, 28, 5, 5}, torch::kFloat32);
    auto tensor_acc = cpu_tensor.accessor<float, 4>(); // [batch][channel][row][col]

    // --- Planes 0-3: Map blocks/heights ---
    auto blocks = b.get_blocks();
    for (int i = 0; i < 25; ++i) {
        int h = blocks[i];
        if (h >= 1 && h <= 4) {
            tensor_acc[0][h - 1][i / 5][i % 5] = 1.0f;
        }
    }

    // --- Setup Active vs Waiting Player ---
    int my_god_idx, opp_god_idx;
    std::array<int, 2> my_workers, opp_workers;
    float is_player_1 = (b.get_turn() == 1) ? 1.0f : 0.0f;

    auto workers = b.get_workers();
    auto gods = b.get_gods();

    if (b.get_turn() == 1) { // Gray's turn
        my_workers = {workers[0], workers[1]};
        opp_workers = {workers[2], workers[3]};
        my_god_idx = static_cast<int>(gods[0]);
        opp_god_idx = static_cast<int>(gods[1]);
    } else { // Blue's turn
        my_workers = {workers[2], workers[3]};
        opp_workers = {workers[0], workers[1]};
        my_god_idx = static_cast<int>(gods[1]);
        opp_god_idx = static_cast<int>(gods[0]);
    }

    // --- Planes 4-5: Fill workers ---
    for (int idx : my_workers) tensor_acc[0][4][idx / 5][idx % 5] = 1.0f;
    for (int idx : opp_workers) tensor_acc[0][5][idx / 5][idx % 5] = 1.0f;

    // --- Planes 6-7: Fill Context (Turn and Athena) ---
    bool athena_active = b.get_prevent_up_next_turn();
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 5; ++c) {
            tensor_acc[0][6][r][c] = is_player_1;
            if (athena_active) {
                tensor_acc[0][7][r][c] = 1.0f;
            }
        }
    }

    // --- Planes 8-27: Fill Gods ---
    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 5; ++c) {
            tensor_acc[0][8 + my_god_idx][r][c] = 1.0f;
            tensor_acc[0][18 + opp_god_idx][r][c] = 1.0f;
        }
    }

    // 2. Move to device and evaluate
    torch::Tensor state_tensor = cpu_tensor;
    state_tensor = cpu_tensor.to(torch::kCUDA);


    auto& module = get_evaluator_model();
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(state_tensor);

    // Disable autograd for inference speed
    torch::NoGradGuard no_grad;

    // Forward pass
    torch::Tensor output = module.forward(inputs).toTensor();
    float current_player_score_logit = output.item<float>();

    // 3. Perspective formatting
    int score = static_cast<int>(current_player_score_logit * 100.0f);

    return (b.get_turn() == 1) ? score : -score;
}

} // namespace Santorini