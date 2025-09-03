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

    // auto pos = "0N0N0N0N0N0N0G0B1G2N0N0N0B0N0N0N0N1N0N0N0N0N0N0N0N1890";
    // Santorini::Board board = Santorini::Board(pos);
    // Santorini::TranspositionTable tt = Santorini::TranspositionTable();
    // auto mv = Santorini::get_best_move(board, 1000000, tt, 4);
    // std::cout << mv->to_text(board) << std::endl;
    return 0;
}