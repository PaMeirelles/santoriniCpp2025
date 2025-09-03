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

    // auto pos = "0N0N0N0N0N0N0G0B0N0N0N0B0G0N0N0N0N0N0N0N0N0N0N0N0N0600";
    // Santorini::Board board = Santorini::Board(pos);
    // // Santorini::TranspositionTable tt = Santorini::TranspositionTable();
    // for (int i=0; i < 100000; i++) auto mvs = board.generate_moves();
    // // auto mv = Santorini::get_best_move(board, 60000, tt);
    // // std::cout << mv->to_text(board) << std::endl;
    return 0;
}