#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <optional>
#include <memory>
#include <vector>
#include <deque>
#include <algorithm>

#include "board.h"
#include "search.h"

namespace Santorini {

class SantoriniEngine {
public:
    SantoriniEngine() = default;

    std::string sq_to_text(int sq) const {
        if (sq < 0 || sq > 24) return "";
        char r = 'a' + (sq % 5);
        char c = '1' + (sq / 5);
        return std::string({r, c});
    }

    std::string translate_sequence(const std::vector<int>& seq, const Board& original_board) const {
        if (seq.empty()) return "none";

        int from_sq = -1, to_sq = -1, build1 = -1, build2 = -1;
        bool dome = false;

        for (int act : seq) {
            if (act >= Board::MOVE_OFFSET && act < Board::BUILD_1_OFFSET) {
                from_sq = act / 25;
                to_sq = act % 25;
            } else if (act >= Board::BUILD_1_OFFSET && act < Board::BUILD_2_OFFSET) {
                build1 = act - Board::BUILD_1_OFFSET;
            } else if (act >= Board::BUILD_2_OFFSET && act <= 674) {
                build2 = act - Board::BUILD_2_OFFSET;
            } else if (act == Board::DOME_ACTION) {
                dome = true;
            }
        }

        if (from_sq == -1 || to_sq == -1) return "none";

        Constants::God god = original_board.get_current_god();
        std::string res = sq_to_text(from_sq);

        // Reconstruct missing path for Artemis
        if (god == Constants::God::ARTEMIS && !original_board.move_checks(from_sq, to_sq)) {
            for (int mid : Constants::NEIGHBOURS[from_sq]) {
                if (original_board.move_checks(from_sq, mid) && original_board.move_checks(mid, to_sq) && original_board.is_free(mid)) {
                    res += sq_to_text(mid);
                    break;
                }
            }
        }
        // Reconstruct missing path for Hermes
        else if (god == Constants::God::HERMES && !adj_ok(from_sq, to_sq) && from_sq != to_sq) {
            std::vector<int> parent(25, -1);
            std::deque<int> q;
            q.push_back(from_sq);
            int h = original_board.get_blocks()[from_sq];
            bool found = false;

            while(!q.empty()) {
                int curr = q.front(); q.pop_front();
                if (curr == to_sq) { found = true; break; }
                for(int nxt : Constants::NEIGHBOURS[curr]) {
                    if (parent[nxt] == -1 && original_board.is_free(nxt) && original_board.get_blocks()[nxt] == h) {
                        parent[nxt] = curr;
                        q.push_back(nxt);
                    }
                }
            }
            if (found) {
                std::vector<int> path;
                int curr = parent[to_sq];
                while(curr != from_sq) {
                    path.push_back(curr);
                    curr = parent[curr];
                }
                std::reverse(path.begin(), path.end());
                for (int p : path) {
                    res += sq_to_text(p);
                }
            }
        }

        res += sq_to_text(to_sq);
        if (build1 != -1) {
            res += sq_to_text(build1);
        }

        // Append God specific extra build components
        if (god == Constants::God::PROMETHEUS && build2 != -1) {
            res += sq_to_text(build2);
        } else if (god == Constants::God::DEMETER && build2 != -1) {
            res += sq_to_text(build2);
        } else if (god == Constants::God::HEPHAESTUS && build2 != -1) {
            res += sq_to_text(build2);
        } else if (god == Constants::God::ATLAS && dome) {
            res += "D";
        }

        return res;
    }

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
                if (std::getline(ss, position_str)) {
                    position_str.erase(0, position_str.find_first_not_of(" \t\n\r"));
                    try {
                        board.emplace(position_str);
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

                SearchResult result = get_best_move(*board, remaining_time_ms, depth_opt);

                if (output_score) {
                    std::cout << "info score " << result.score << std::endl;
                }
                if (output_nodes) {
                    std::cout << "info nodes " << result.nodes << std::endl;
                }

                std::string bestmove_str = translate_sequence(result.best_action_sequence, *board);
                std::cout << "bestmove " << bestmove_str << std::endl;

            } else if (command == "quit") {
                break;
            } else if (!command.empty()) {
                std::cerr << "Unknown command: " << command << std::endl;
            }
        }
    }

private:
    std::optional<Board> board;
};

} // namespace Santorini