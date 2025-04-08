#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <cstdlib>

namespace sevens {

struct Card {
    int suit;
    int rank;
};

class MyCardParser {
public:
    std::unordered_map<int, Card> cards_hashmap;
    void read_cards(const std::string& filename);
};

}

namespace {
int convertRank(const std::string& r) {
    if (r == "Jack") return 11;
    if (r == "Queen") return 12;
    if (r == "King") return 13;
    if (r == "Ace") return 1;
    return std::stoi(r);
}
int convertSuit(const std::string& s) {
    if (s == "Clubs") return 0;
    if (s == "Diamonds") return 1;
    if (s == "Hearts") return 2;
    if (s == "Spades") return 3;
    return -1;
}
std::string suitToString(int suit) {
    switch (suit) {
        case 0: return "Clubs";
        case 1: return "Diamonds";
        case 2: return "Hearts";
        case 3: return "Spades";
        default: return "Unknown";
    }
}
std::string rankToString(int rank) {
    if (rank == 1) return "Ace";
    if (rank == 11) return "Jack";
    if (rank == 12) return "Queen";
    if (rank == 13) return "King";
    return std::to_string(rank);
}
}

namespace sevens {

void MyCardParser::read_cards(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: cannot open file " << filename << "\n";
        return;
    }
    int id = 1;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string rankStr, ofStr, suitStr;
        if (!(iss >> rankStr >> ofStr >> suitStr)) continue;
        int rank = convertRank(rankStr);
        int suit = convertSuit(suitStr);
        if (suit == -1) continue;
        cards_hashmap[id] = Card{suit, rank};
        ++id;
    }
    std::cout << "[MyCardParser::read_cards] " << id << " cards read from " << filename << "\n";
}

}

int main() {
    sevens::MyCardParser parser;
    parser.read_cards("cards.txt");
    for (const auto& entry : parser.cards_hashmap) {
        int id = entry.first;
        sevens::Card card = entry.second;
        std::cout << "Card " << id << ": " << rankToString(card.rank) << " of " << suitToString(card.suit) << "\n";
    }
    return 0;
}
