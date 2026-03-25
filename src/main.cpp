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

    return 0;
}