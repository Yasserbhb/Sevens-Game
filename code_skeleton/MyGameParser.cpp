// MyGameParser.cpp
#include "MyGameParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace sevens {

void MyGameParser::read_game(const std::string& /*filename*/) {
    // Initialize the table layout for all suits
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 1; rank <= 13; rank++) {
            table_layout[suit][rank] = false;
        }
    }
    
    // The game starts with the 7 of Diamonds on the table
    table_layout[1][7] = true; // 1 = Diamonds, 7 = Seven
    
    std::cout << "Game initialized with 7 of Diamonds on the table" << std::endl;
}

}