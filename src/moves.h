#pragma once



#include <string>

#include <vector>

#include <memory>

#include <stdexcept>

#include <optional>

#include <numeric>

#include <utility>



#include "constants.h" // Assumes constants.h is in the include path

using namespace std;

namespace Santorini::Moves {

// #############################################################################

// # Utility Functions

// #############################################################################



inline sq_i text_to_square(const string& square_text) {

if (square_text.length() != 2) {

throw invalid_argument("Square text must be 2 characters long: " + square_text);

}

int row = square_text[0] - 'a';

int col = square_text[1] - '1';

if (row < 0 || row > 4 || col < 0 || col > 4) {

throw invalid_argument("Invalid square format: " + square_text);

}

return static_cast<sq_i>(col * 5 + row);

}



inline string square_to_text(sq_i square) {

if (square < 0 || square > 24) {

throw out_of_range("Square index out of range: " + to_string(square));

}

char row = (square % 5) + 'a';

char col = (square / 5) + '1';

return {row, col};

}



// #############################################################################

// # Abstract Base Class

// #############################################################################



class Move {

public:

sq_i from_sq;

bool had_athena_flag = false;

int score = 0;

Constants::God god;



Move(sq_i from_sq, Constants::God god) : from_sq(from_sq), god(god) {}

virtual ~Move() = default;



virtual sq_i final_sq() const = 0;

virtual string to_text() const = 0;

};



// #############################################################################

// # God-specific move types

// #############################################################################



class ApolloMove : public Move {

public:

sq_i to_sq;

sq_i build_sq;



ApolloMove(sq_i from_sq, sq_i to_sq, sq_i build_sq, Constants::God god_type = Constants::God::APOLLO)

: Move(from_sq, god_type), to_sq(to_sq), build_sq(build_sq) {}



sq_i final_sq() const override { return to_sq; }



string to_text() const override {

return square_to_text(from_sq) + square_to_text(to_sq) + square_to_text(build_sq);

}



static unique_ptr<ApolloMove> from_text(const string& move_text) {

return make_unique<ApolloMove>(

text_to_square(move_text.substr(0, 2)),

text_to_square(move_text.substr(2, 2)),

text_to_square(move_text.substr(4, 2))

);

}

};



class ArtemisMove : public Move {

public:

sq_i to_sq;

sq_i build_sq;

optional<sq_i> mid_sq;



ArtemisMove(sq_i from, sq_i to, sq_i build, optional<sq_i> mid = nullopt)

: Move(from, Constants::God::ARTEMIS), to_sq(to), build_sq(build), mid_sq(mid) {}



sq_i final_sq() const override { return to_sq; }



string to_text() const override {

string text = square_to_text(from_sq);

if (mid_sq) text += square_to_text(*mid_sq);

text += square_to_text(to_sq);

text += square_to_text(build_sq);

return text;

}



static unique_ptr<ArtemisMove> from_text(const string& move_text) {

if (move_text.length() == 6) {

return make_unique<ArtemisMove>(

text_to_square(move_text.substr(0, 2)),

text_to_square(move_text.substr(2, 2)),

text_to_square(move_text.substr(4, 2))

);

}

if (move_text.length() == 8) {

return make_unique<ArtemisMove>(

text_to_square(move_text.substr(0, 2)),

text_to_square(move_text.substr(4, 2)),

text_to_square(move_text.substr(6, 2)),

text_to_square(move_text.substr(2, 2))

);

}

throw invalid_argument("ArtemisMove text must be 6 or 8 chars.");

}

};



class HermesMove : public Move {

public:

vector<sq_i> squares;

sq_i build_sq;



HermesMove(sq_i from, vector<sq_i> sqs, sq_i build)

: Move(from, Constants::God::HERMES), squares(move(sqs)), build_sq(build) {}



sq_i final_sq() const override { return squares.empty() ? from_sq : squares.back(); }



string to_text() const override {

string text = square_to_text(from_sq);

for (sq_i sq : squares) text += square_to_text(sq);

text += square_to_text(build_sq);

return text;

}



static unique_ptr<HermesMove> from_text(const string& move_text) {

if (move_text.length() < 4 || (move_text.length() % 2) != 0) {

throw invalid_argument("HermesMove must be even length >= 4");

}

sq_i from = text_to_square(move_text.substr(0, 2));

sq_i build = text_to_square(move_text.substr(move_text.length() - 2));

string middle = move_text.substr(2, move_text.length() - 4);

vector<sq_i> mid_squares;

for (size_t i = 0; i < middle.length(); i += 2) {

mid_squares.push_back(text_to_square(middle.substr(i, 2)));

}

return make_unique<HermesMove>(from, mid_squares, build);

}

};



class DemeterMove : public Move {

public:

sq_i to_sq;

sq_i build_sq_1;

optional<sq_i> build_sq_2;



DemeterMove(sq_i from, sq_i to, sq_i b1, optional<sq_i> b2 = nullopt, Constants::God god_type = Constants::God::DEMETER)

: Move(from, god_type), to_sq(to), build_sq_1(b1), build_sq_2(b2) {}



sq_i final_sq() const override { return to_sq; }



string to_text() const override {

string text = square_to_text(from_sq) + square_to_text(to_sq) + square_to_text(build_sq_1);

if (build_sq_2) text += square_to_text(*build_sq_2);

return text;

}



static unique_ptr<DemeterMove> from_text(const string& move_text) {

if (move_text.length() == 6) {

return make_unique<DemeterMove>(

text_to_square(move_text.substr(0, 2)),

text_to_square(move_text.substr(2, 2)),

text_to_square(move_text.substr(4, 2))

);

}

if (move_text.length() == 8) {

return make_unique<DemeterMove>(

text_to_square(move_text.substr(0, 2)),

text_to_square(move_text.substr(2, 2)),

text_to_square(move_text.substr(4, 2)),

text_to_square(move_text.substr(6, 2))

);

}

throw invalid_argument("DemeterMove must be 6 or 8 chars.");

}

};



class HephaestusMove : public DemeterMove {

public:

HephaestusMove(sq_i from, sq_i to, sq_i b1, optional<sq_i> b2 = nullopt)

: DemeterMove(from, to, b1, b2, Constants::God::HEPHAESTUS) {}

};



class PanMove : public ApolloMove {

public:

PanMove(sq_i from, sq_i to, sq_i build)

: ApolloMove(from, to, build, Constants::God::PAN) {}

};



class PrometheusMove : public Move {

public:

sq_i to_sq;

sq_i build_sq;

optional<sq_i> optional_build;



PrometheusMove(sq_i from, sq_i to, sq_i build, optional<sq_i> opt_build = nullopt)

: Move(from, Constants::God::PROMETHEUS), to_sq(to), build_sq(build), optional_build(opt_build) {}



sq_i final_sq() const override { return to_sq; }



string to_text() const override {

string text = square_to_text(from_sq) + square_to_text(to_sq) + square_to_text(build_sq);

if (optional_build) text += square_to_text(*optional_build);

return text;

}

};



class AthenaMove : public ApolloMove {

public:

AthenaMove(sq_i from, sq_i to, sq_i build)

: ApolloMove(from, to, build, Constants::God::ATHENA) {}

};



class MinotaurMove : public ApolloMove {

public:

bool pushed = false;

MinotaurMove(sq_i from, sq_i to, sq_i build)

: ApolloMove(from, to, build, Constants::God::MINOTAUR) {}

};



class AtlasMove : public Move {

public:

sq_i to_sq;

sq_i build_sq;

bool dome;

optional<sq_i> orig_h;



AtlasMove(sq_i from, sq_i to, sq_i build, bool has_dome, optional<sq_i> h = nullopt)

: Move(from, Constants::God::ATLAS), to_sq(to), build_sq(build), dome(has_dome), orig_h(h) {}



sq_i final_sq() const override { return to_sq; }



string to_text() const override {

string text = square_to_text(from_sq) + square_to_text(to_sq) + square_to_text(build_sq);

if (dome) text += "D";

return text;

}

};



} // namespace Santorini::Moves