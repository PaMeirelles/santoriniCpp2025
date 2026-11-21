#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include <memory>

#include "board.h"
#include "transposition_table.h"
#include "search.h"

namespace Santorini {

class SantoriniEngine {
public:
    SantoriniEngine() = default;

    void run() {
        std::string line;
        while (std::getline(std::cin, line)) {
            std::stringstream ss(line);
            std::string command;
            ss >> command;

            if (command == "isready") {
                std::cout << "info capability elapsedply" << std::endl;
                std::cout << "readyok" << std::endl;
            } else if (command == "position") {
                std::string position_str;
                if (std::getline(ss, position_str)) {
                    position_str.erase(0, position_str.find_first_not_of(" \t\n\r"));
                    try {
                        board.emplace(position_str);
                        tt.clear();
                        std::cout << "Position set." << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error setting position: " << e.what() << std::endl;
                    }
                } else {
                    std::cerr << "Error: Missing position argument." << std::endl;
                }
            } else if (command == "go" || command == "clock") {
                if (!board) {
                    std::cout << "bestmove none" << std::endl;
                    continue;
                }

                std::optional<int> elapsed_plies;
                bool clock_info = command == "clock";
                bool invalid_ply = false;
                int gtime = 1000;
                int btime = 1000;
                bool output_score = false;
                bool output_nodes = false;
                std::optional<int> depth_opt = std::nullopt; // For depth-limited search

                std::string token;
                while (ss >> token) {
                    if (token == "ply") {
                        int value = -1;
                        if (!(ss >> value) || value < 0) { invalid_ply = true; break; }
                        elapsed_plies = value;
                    } else if (token == "clockinfo") {
                        clock_info = true;
                    } else if (token == "gtime") {
                        ss >> gtime;
                    } else if (token == "btime") {
                        ss >> btime;
                    } else if (token == "score") {
                        output_score = true;
                    } else if (token == "nodes") {
                        output_nodes = true;
                    } else if (token == "depth") { // New: Parse depth command
                        int d;
                        if (ss >> d) {
                            depth_opt = d;
                        }
                    }
                }

                // If depth is set, use a very large time limit. Otherwise, use game time.
                int remaining_time_ms = 1000 * 60 * 60; // 1 hour
                if (!depth_opt.has_value()) {
                    remaining_time_ms = (board->get_turn() == 1) ? gtime : btime;
                }

                if (invalid_ply || (!depth_opt && !elapsed_plies)) {
                    std::cout << "info string Aura requires go ply <nonnegative elapsed plies>" << std::endl;
                    if (command == "go") std::cout << "bestmove none" << std::endl;
                    continue;
                }
                if (clock_info) {
                    const int ply = elapsed_plies.value_or(0);
                    std::cout << "info clock ply " << ply
                              << " expected_own_turns " << AuraClock::expected_own_turns(ply)
                              << " budget_ms " << (depth_opt ? remaining_time_ms / 10 : AuraClock::budget_ms(remaining_time_ms, ply))
                              << std::endl;
                }
                if (command == "clock") continue;
                // Pass explicit game ply; Board is reconstructed on position updates.
                SearchResult result = get_best_move(*board, remaining_time_ms, tt, depth_opt, elapsed_plies);

                if (output_score) {
                    std::cout << "info score " << result.score << std::endl;
                }
                if (output_nodes) {
                    std::cout << "info nodes " << result.nodes << std::endl;
                }

                if (!result.best_move) {
                    std::cout << "bestmove none" << std::endl;
                } else {
                    std::cout << "bestmove " << result.best_move->to_text(*board) << std::endl;
                }

            } else if (command == "quit") {
                break;
            } else if (!command.empty()) {
                std::cerr << "Unknown command: " << command << std::endl;
            }
        }
    }

private:
    std::optional<Board> board;
    TranspositionTable tt;
};

} // namespace Santorini
