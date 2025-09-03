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

    // auto pos = "0N0N0N0N0N0N0G1N0N0N1N0B0G0B0N0N1N1N0N0N0N0N0N0N0N0600";
    // Santorini::Board board = Santorini::Board(pos);
    // Santorini::TranspositionTable tt = Santorini::TranspositionTable();
    // auto mv = Santorini::get_best_move(board, 60000, tt, 5);
    // std::cout << mv->to_text(board) << std::endl;
    return 0;
}
