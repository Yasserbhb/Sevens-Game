// RandomStrategy.cpp
#include "RandomStrategy.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
#include <iostream>

namespace sevens {

// Constructor seeds the RNG
RandomStrategy::RandomStrategy() {
    auto seed = static_cast<unsigned long>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    rng.seed(seed);
}

void RandomStrategy::initialize(uint64_t playerID) {
    myID = playerID;
}

int RandomStrategy::selectCardToPlay(
    const std::vector<Card>& hand,
    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout)
{
    // If our hand is empty, we can't play
    if (hand.empty()) {
        return -1; // pass
    }

    // Find playable cards in the hand
    std::vector<int> playableCardIndices;
    for (int i = 0; i < static_cast<int>(hand.size()); i++) {
        const Card& card = hand[i];
        int suit = card.suit;
        int rank = card.rank;
        
        // Skip cards we know are not playable
        // Special case: 7s can be played only if not already on the table
        if (rank == 7) {
            // Check if this 7 is already on the table
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(rank) > 0 && 
                tableLayout.at(suit).at(rank)) {
                // This 7 is already on the table, cannot play it
                continue;
            } else {
                // This 7 is not on the table, can play it
                playableCardIndices.push_back(i);
                continue;
            }
        }
        
        // Check if rank+1 is on the table
        bool higher = (rank < 13 && 
                      tableLayout.count(suit) > 0 && 
                      tableLayout.at(suit).count(rank+1) > 0 && 
                      tableLayout.at(suit).at(rank+1));
                      
        // Check if rank-1 is on the table
        bool lower = (rank > 1 && 
                     tableLayout.count(suit) > 0 && 
                     tableLayout.at(suit).count(rank-1) > 0 && 
                     tableLayout.at(suit).at(rank-1));
        
        if (higher || lower) {
            playableCardIndices.push_back(i);
        }
    }
    
    // If no playable cards, pass
    if (playableCardIndices.empty()) {
        return -1;
    }
    
    // Select a random playable card
    std::uniform_int_distribution<int> dist(0, static_cast<int>(playableCardIndices.size()) - 1);
    int randomIndex = dist(rng);
    return playableCardIndices[randomIndex];
}

void RandomStrategy::observeMove(uint64_t /*playerID*/, const Card& /*playedCard*/) {
    // This simplified strategy ignores other players' moves
}

void RandomStrategy::observePass(uint64_t /*playerID*/) {
    // This simplified strategy ignores passes
}

std::string RandomStrategy::getName() const {
    return "RandomStrategy";
}

} // namespace sevens

// Add this at the end of RandomStrategy.cpp, after the namespace closing brace

// At the end of RandomStrategy.cpp, after the namespace closing brace

#ifdef BUILDING_DLL
extern "C" {
    // Export function for dynamic library loading - works on both Windows and Linux
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    sevens::PlayerStrategy* createStrategy() {
        return new sevens::RandomStrategy();
    }
}
#endif