#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "run.h"
#include "search.h"


int main() {
    // std::ios_base::sync_with_stdio(false);
    // std::cin.tie(NULL);
    //
    // Santorini::SantoriniEngine engine;
    // engine.run();

    auto pos = "1N1G0N0N0N0N0N0G0N0N0N0N0B0N0N0B1N0N0N0N0N0N0N0N0N1211";
    Santorini::Board board = Santorini::Board(pos);
    Santorini::TranspositionTable tt = Santorini::TranspositionTable();
    auto mv = Santorini::get_best_move(board, 1000000, tt, 5);
    cout << mv->to_text() << endl;
    return 0;
}
