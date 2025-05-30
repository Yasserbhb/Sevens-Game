// MyCardParser.cpp
#include "MyCardParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sevens {

void MyCardParser::read_cards(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: cannot open file " << filename << std::endl;
        return;
    }

    // Clear existing cards
    cards_hashmap.clear();
    
    // Helper functions to convert text to numeric values
    auto convertRank = [](const std::string& r) -> int {
        if (r == "Jack") return 11;
        if (r == "Queen") return 12;
        if (r == "King") return 13;
        if (r == "Ace") return 1;
        return std::stoi(r);
    };
    
    auto convertSuit = [](const std::string& s) -> int {
        if (s == "Clubs") return 0;
        if (s == "Diamonds") return 1;
        if (s == "Hearts") return 2;
        if (s == "Spades") return 3;
        return -1;
    };
    
    // Read each line from the file
    std::string line;
    uint64_t id = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string rankStr, ofStr, suitStr;
        
        if (!(iss >> rankStr >> ofStr >> suitStr) || ofStr != "of") {
            std::cerr << "Warning: invalid card format: " << line << std::endl;
            continue;
        }
        
        int rank = convertRank(rankStr);
        int suit = convertSuit(suitStr);
        
        if (suit < 0 || rank < 1 || rank > 13) {
            std::cerr << "Warning: invalid card values: " << line << std::endl;
            continue;
        }
        
        cards_hashmap[id] = Card{suit, rank};
        id++;
    }
    
    std::cout << "Parsed " << cards_hashmap.size() << " cards from " << filename << std::endl;
}

} // namespace sevens