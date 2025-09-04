#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "run.h"
#include "search.h"


int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    // Santorini::SantoriniEngine engine;
    // engine.run();

    auto pos = "0N0G0N0N0N0B0N0N0N0N0N0N0N0N0B0N0N0N0N0N0N0N0G0N0N0400";
    Santorini::Board board = Santorini::Board(pos);
    Santorini::TranspositionTable tt = Santorini::TranspositionTable();
    // for (int i=0; i < 100000; i++) auto mvs = board.generate_moves();
    auto start_time = std::chrono::high_resolution_clock::now();
    auto mv = get_best_move(board, 60000000, tt, 5);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "get_best_move() execution time: " << duration_ms.count() << " milliseconds" << std::endl;
    std::cout << mv->to_text(board) << std::endl;
    return 0;
}