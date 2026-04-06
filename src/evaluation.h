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

    inline fs::path get_executable_dir() {
#ifdef _WIN32
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return fs::path(path).parent_path();
#else
        return fs::read_symlink("/proc/self/exe").parent_path();
#endif
    }

    inline torch::jit::script::Module& get_evaluator_model() {
        static torch::jit::script::Module module;
        static bool is_loaded = false;

        if (!is_loaded) {
            try {
                fs::path model_path = get_executable_dir() / "santorini_evaluator.pt";

                if (!fs::exists(model_path)) {
                    std::cerr << "[ERROR] Model file not found at: " << model_path.string() << "\n";
                    std::exit(-1);
                }

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

    struct NNOutput {
        float value;
        std::array<float, 677> policy_probs;
    };

    // Make sure it takes phase_idx
    inline NNOutput evaluate_board_nn(const Board& b, int phase_idx) {
        // STATIC tensor prevents thousands of memory allocations per second
        static torch::Tensor cpu_tensor = torch::zeros({1, 31, 5, 5}, torch::kFloat32);
        cpu_tensor.zero_(); // Clear previous iteration's data
        auto tensor_acc = cpu_tensor.accessor<float, 4>();

        auto blocks = b.get_blocks();
        for (int i = 0; i < 25; ++i) {
            int h = blocks[i];
            if (h >= 1 && h <= 4) {
                tensor_acc[0][h - 1][i / 5][i % 5] = 1.0f;
            }
        }

        int my_god_idx, opp_god_idx;
        std::array<int, 2> my_workers, opp_workers;
        float is_player_1 = (b.get_turn() == 1) ? 1.0f : 0.0f;

        auto workers = b.get_workers();
        auto gods = b.get_gods();

        if (b.get_turn() == 1) {
            my_workers = {workers[0], workers[1]};
            opp_workers = {workers[2], workers[3]};
            my_god_idx = static_cast<int>(gods[0]);
            opp_god_idx = static_cast<int>(gods[1]);
        } else {
            my_workers = {workers[2], workers[3]};
            opp_workers = {workers[0], workers[1]};
            my_god_idx = static_cast<int>(gods[1]);
            opp_god_idx = static_cast<int>(gods[0]);
        }

        for (int idx : my_workers) tensor_acc[0][4][idx / 5][idx % 5] = 1.0f;
        for (int idx : opp_workers) tensor_acc[0][5][idx / 5][idx % 5] = 1.0f;

        bool athena_active = b.get_prevent_up_next_turn();
        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 5; ++c) {
                tensor_acc[0][6][r][c] = is_player_1;
                if (athena_active) tensor_acc[0][7][r][c] = 1.0f;
                // NO MORE OUT OF BOUNDS WRITE HERE
            }
        }

        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 5; ++c) {
                tensor_acc[0][8 + my_god_idx][r][c] = 1.0f;
                tensor_acc[0][18 + opp_god_idx][r][c] = 1.0f;
            }
        }

        // Apply correct Phase plane
        if (phase_idx >= 0 && phase_idx <= 2) {
            for (int r = 0; r < 5; ++r) {
                for (int c = 0; c < 5; ++c) {
                    tensor_acc[0][28 + phase_idx][r][c] = 1.0f;
                }
            }
        }

        torch::Tensor state_tensor = cpu_tensor.to(torch::kCUDA);
        auto& module = get_evaluator_model();
        std::vector<torch::jit::IValue> inputs{state_tensor};

        torch::NoGradGuard no_grad;
        auto outputs = module.forward(inputs).toTuple();

        torch::Tensor policy_logits = outputs->elements()[0].toTensor();
        torch::Tensor value_logit   = outputs->elements()[1].toTensor();

        float p = torch::sigmoid(value_logit).item<float>();
        float v = (p * 2.0f) - 1.0f;

        torch::Tensor policy_probs = torch::softmax(policy_logits, 1).cpu();
        auto policy_acc = policy_probs.accessor<float, 2>();

        NNOutput result;
        result.value = v;
        for (int i = 0; i < 677; ++i) {
            result.policy_probs[i] = policy_acc[0][i];
        }

        return result;
    }

} // namespace Santorini