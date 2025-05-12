#pragma once

#include "Generic_game_parser.hpp"

namespace sevens {

/**
 * Derived from Generic_game_parser.
 * Subclasses must fill in read_game(...) to initialize the table layout.
 */
class MyGameParser : public Generic_game_parser {
public:
    MyGameParser() = default;
    ~MyGameParser() = default;

    void read_game(const std::string& filename) override;
    void read_cards(const std::string& filename) override; // Add this
};

} // namespace sevens
