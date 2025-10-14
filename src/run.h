#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <optional>
#include <memory>

#include "board.h"
#include "transposition_table.h"
#include "search.h"
#include "search_params.h"

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
                std::cout << "readyok" << std::endl;
            } else if (command == "setoption") {
                handle_set_option(ss);
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
            } else if (command == "go") {
                if (!board) {
                    std::cout << "bestmove none" << std::endl;
                    continue;
                }

                int gtime = 1000;
                int btime = 1000;
                bool output_score = false;
                bool output_nodes = false;
                std::optional<int> depth_opt = std::nullopt;

                std::string token;
                while (ss >> token) {
                    if (token == "gtime") {
                        ss >> gtime;
                    } else if (token == "btime") {
                        ss >> btime;
                    } else if (token == "score") {
                        output_score = true;
                    } else if (token == "nodes") {
                        output_nodes = true;
                    } else if (token == "depth") {
                        int d;
                        if (ss >> d) {
                            depth_opt = d;
                        }
                    }
                }

                int remaining_time_ms = 1000 * 60 * 60; // 1 hour
                if (!depth_opt.has_value()) {
                    remaining_time_ms = (board->get_turn() == 1) ? gtime : btime;
                }

                // Pass the params object to the search function
                SearchResult result = get_best_move(*board, remaining_time_ms, tt, params, depth_opt);

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
    SearchParams params; // The engine now owns the search parameters

    void handle_set_option(std::stringstream& ss) {
        std::string name_token, name, value_token, value_str;
        ss >> name_token >> name >> value_token;
        std::getline(ss, value_str); // Read the rest of the line as the value
        value_str.erase(0, value_str.find_first_not_of(" \t\n\r"));


        if (name_token != "name" || value_token != "value") {
            std::cerr << "Error: Malformed setoption command. Use: setoption name <NAME> value <VALUE>" << std::endl;
            return;
        }

        try {
            if (name == "ASP_WINDOW") params.asp_window = std::stoi(value_str);
            else if (name == "MIN_DEPTH") params.min_depth = std::stoi(value_str);
            else if (name == "N_PROTECTED_MOVES") params.n_protected_moves = std::stoi(value_str);
            else if (name == "LMR_FACTOR") params.lmr_factor = std::stof(value_str);
            else if (name == "RFP_DEPTH") params.rfp_depth = std::stoi(value_str);
            else if (name == "RFP_MARGIN") params.rfp_margin = std::stoi(value_str);
            else if (name == "FUTILITY_PRUNING_MAX_DEPTH") params.futility_pruning_max_depth = std::stoi(value_str);
            else if (name == "FUTILITY_BASE_MARGIN") params.futility_base_margin = std::stoi(value_str);
            else if (name == "FUTILITY_DEPTH_MARGIN") params.futility_depth_margin = std::stoi(value_str);
            else if (name == "RAZOR_DEPTH") params.razor_depth = std::stoi(value_str);
            else if (name == "RAZOR_MARGIN") params.razor_margin = std::stoi(value_str);
            else {
                std::cerr << "Unknown option: " << name << std::endl;
                return;
            }
            std::cout << "info string Set " << name << " to " << value_str << std::endl;
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: Invalid value for option " << name << ": " << value_str << std::endl;
        }
    }
};

} // namespace Santorini