#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "run.h"
#include "search.h"


int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    Santorini::SantoriniEngine engine;
    engine.run();

    // auto pos = "0N0N4N1N3N0N4N1N1G1N4N4N4N4N1B2N1N1B3N0N4N1N0G1N0N1310";
    // Santorini::Board board = Santorini::Board(pos);
    // Santorini::TranspositionTable tt = Santorini::TranspositionTable();
    // // // for (int i=0; i < 100000; i++) auto mvs = board.generate_moves();
    // auto start_time = std::chrono::high_resolution_clock::now();
    // auto mv = get_best_move(board, 60000000, tt, 10);
    // auto end_time = std::chrono::high_resolution_clock::now();
    // auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "get_best_move() execution time: " << duration_ms.count() << " milliseconds" << std::endl;
    // std::cout << mv->to_text(board) << std::endl;
    return 0;
}