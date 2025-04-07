#include "MyCardParser.hpp"
#include <iostream>

namespace sevens {

void MyCardParser::read_cards(const std::string& /*filename*/) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Erreur : impossible d’ouvrir le fichier " << filename << "\n";
        return;
    }

    int id = 0;
    int suit, rank;
    while (file >> suit >> rank) {
        cards_hashmap[id] = Card{suit, rank};
        ++id;
    }

    std::cout << "[MyCardParser::read_cards] " << id << " cartes lues depuis " << filename << "\n";
}

} // namespace sevens
