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
                std::cout << "readyok" << std::endl;
            } else if (command == "position") {
                std::string position_str;
                // Read the rest of the line as the position string
                if (std::getline(ss, position_str)) {
                    // Remove leading whitespace if any
                    position_str.erase(0, position_str.find_first_not_of(" \t\n\r"));
                    try {
                        board.emplace(position_str);
                        std::cout << "Position set." << std::endl; // Optional debug message
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
                std::string token;
                while (ss >> token) {
                    if (token == "gtime") {
                        ss >> gtime;
                    } else if (token == "btime") {
                        ss >> btime;
                    }
                }

                int remaining_time_ms = (board->get_turn() == 1) ? gtime : btime;

                std::unique_ptr<Moves::Move> best_move = get_best_move(*board, remaining_time_ms, tt);

                if (!best_move) {
                    std::cout << "bestmove none" << std::endl;
                } else {
                    std::cout << "bestmove " << best_move->to_text(*board) << std::endl;
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
