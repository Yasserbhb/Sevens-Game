// GreedyStrategy.cpp
#include "GreedyStrategy.hpp"
#include <algorithm>
#include <iostream>

namespace sevens {

void GreedyStrategy::initialize(uint64_t playerID) {
    myID = playerID;
}

int GreedyStrategy::selectCardToPlay(
    const std::vector<Card>& hand,
    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout)
{
    // If the hand is empty, pass
    if (hand.empty()) {
        return -1;
    }
    
    // Find the first playable card in the hand
    for (int i = 0; i < static_cast<int>(hand.size()); i++) {
        const Card& card = hand[i];
        int suit = card.suit;
        int rank = card.rank;
        
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
                return i;
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
            return i;
        }
    }
    
    // No playable card found
    return -1;
}

void GreedyStrategy::observeMove(uint64_t /*playerID*/, const Card& /*playedCard*/) {
    // Ignored in minimal version
}

void GreedyStrategy::observePass(uint64_t /*playerID*/) {
    // Ignored in minimal version
}

std::string GreedyStrategy::getName() const {
    return "GreedyStrategy";
}

} // namespace sevens

// Add this at the end of GreedyStrategy.cpp, after the namespace closing brace

// At the end of GreedyStrategy.cpp, after the namespace closing brace

#ifdef BUILDING_DLL
extern "C" {
    // Export function for dynamic library loading - works on both Windows and Linux
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    sevens::PlayerStrategy* createStrategy() {
        return new sevens::GreedyStrategy(); // CORRECT - return GreedyStrategy
    }
}
#endif