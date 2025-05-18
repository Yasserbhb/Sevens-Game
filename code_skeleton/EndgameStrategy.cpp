#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <chrono>
#include <string>
#include <queue>
#include <memory>

namespace sevens {

/**
 * EndgameStrategy: Focus on optimizing for emptying hand as quickly as possible,
 * with special emphasis on planning moves in advance during the endgame phase.
 */
class EndgameStrategy : public PlayerStrategy {
public:
    EndgameStrategy() {
        // Initialize RNG with time-based seed
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    virtual ~EndgameStrategy() = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        emptyHandPossible = false;
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Increment turn counter
        roundTurn++;
        
        // Find all playable cards
        std::vector<int> playableIndices = findPlayableCards(hand, tableLayout);
        
        // If no playable cards, we must pass
        if (playableIndices.empty()) {
            return -1;
        }
        
        // If only one playable card, play it
        if (playableIndices.size() == 1) {
            return playableIndices[0];
        }
        
        // Determine the game phase
        int gamePhase = getGamePhase(hand);
        
        // If we're in the late game (5 or fewer cards), use advanced planning
        if (gamePhase == 2) {
            return selectLateGameMove(playableIndices, hand, tableLayout);
        }
        
        // For early and mid-game, use strategy that sets up for endgame
        return selectEarlyMidGameMove(playableIndices, hand, tableLayout, gamePhase);
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // We don't need extensive tracking for this strategy
        (void)playerID;
        (void)playedCard;
    }
    
    void observePass(uint64_t playerID) override {
        // We don't need extensive tracking for this strategy
        (void)playerID;
    }
    
    std::string getName() const override {
        return "EndgameStrategy";
    }
    
private:
    // Game state tracking
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    bool emptyHandPossible = false;
    
    // Constants for strategy
    static constexpr int MAX_LOOKAHEAD_DEPTH = 3;  // Maximum depth for endgame planning
    
    // Find all playable cards in hand
    std::vector<int> findPlayableCards(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        std::vector<int> playableIndices;
        
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (isCardPlayable(hand[i], tableLayout)) {
                playableIndices.push_back(i);
            }
        }
        
        return playableIndices;
    }
    
    // Check if a card is playable on the current table
    bool isCardPlayable(
        const Card& card,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        int suit = card.suit;
        int rank = card.rank;
        
        // 7s are playable if not already on the table
        if (rank == 7) {
            return !(tableLayout.count(suit) > 0 && 
                    tableLayout.at(suit).count(rank) > 0 && 
                    tableLayout.at(suit).at(rank));
        }
        
        // Check if rank+1 is on the table (can play a lower card)
        bool higherOnTable = (rank < 13 && 
                            tableLayout.count(suit) > 0 && 
                            tableLayout.at(suit).count(rank+1) > 0 && 
                            tableLayout.at(suit).at(rank+1));
                            
        // Check if rank-1 is on the table (can play a higher card)
        bool lowerOnTable = (rank > 1 && 
                            tableLayout.count(suit) > 0 && 
                            tableLayout.at(suit).count(rank-1) > 0 && 
                            tableLayout.at(suit).at(rank-1));
        
        return higherOnTable || lowerOnTable;
    }
    
    // Get game phase based on hand size
    int getGamePhase(const std::vector<Card>& hand) const {
        if (hand.size() > 10) return 0; // Early game
        if (hand.size() > 5) return 1;  // Mid game
        return 2;                       // Late game
    }
    
    // Check if a card is an extreme card (Ace, 2, 3 or Jack, Queen, King)
    bool isExtremeCard(const Card& card) const {
        return card.rank <= 3 || card.rank >= 11;
    }
    
    // Check if a card is a singleton (only card of its suit in hand)
    bool isSingleton(const Card& card, const std::vector<Card>& hand) const {
        int suitCount = 0;
        for (const Card& c : hand) {
            if (c.suit == card.suit) {
                suitCount++;
            }
        }
        return suitCount == 1;
    }
    
    // Count future plays after playing a card
    int countFuturePlays(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // Create a simulated table with this card played
        auto simulatedTable = tableLayout;
        const Card& playedCard = hand[cardIdx];
        simulatedTable[playedCard.suit][playedCard.rank] = true;
        
        // Count how many of our remaining cards would be playable
        int count = 0;
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (i != cardIdx && isCardPlayable(hand[i], simulatedTable)) {
                count++;
            }
        }
        
        return count;
    }
    
    // Calculate the minimum number of turns to empty hand from this position
    // Returns -1 if impossible to empty hand
    int calculateMinTurnsToEmpty(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        int maxDepth = MAX_LOOKAHEAD_DEPTH) const 
    {
        // If hand is empty, we're done
        if (hand.empty()) {
            return 0;
        }
        
        // Find all playable cards
        std::vector<int> playableIndices;
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (isCardPlayable(hand[i], tableLayout)) {
                playableIndices.push_back(i);
            }
        }
        
        // If no playable cards, we can't empty the hand
        if (playableIndices.empty()) {
            return -1;
        }
        
        // If we've reached max depth, use heuristic
        if (maxDepth <= 0) {
            return static_cast<int>(hand.size());  // Optimistic estimate
        }
        
        // Try each playable card and recursively calculate
        int bestTurns = -1;
        for (int idx : playableIndices) {
            // Create a new hand without this card
            std::vector<Card> newHand;
            for (int i = 0; i < static_cast<int>(hand.size()); i++) {
                if (i != idx) {
                    newHand.push_back(hand[i]);
                }
            }
            
            // Create new table layout with this card played
            auto newTableLayout = tableLayout;
            newTableLayout[hand[idx].suit][hand[idx].rank] = true;
            
            // Recursively calculate
            int turnsAfterPlay = calculateMinTurnsToEmpty(newHand, newTableLayout, maxDepth - 1);
            
            // Update best turns
            if (turnsAfterPlay != -1) {
                int totalTurns = 1 + turnsAfterPlay;  // This turn + future turns
                if (bestTurns == -1 || totalTurns < bestTurns) {
                    bestTurns = totalTurns;
                }
            }
        }
        
        return bestTurns;
    }
    
    // Select the best move for late game (5 or fewer cards)
    int selectLateGameMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // For each playable card, evaluate how many turns it would take to empty hand
        std::vector<std::pair<int, int>> moveEvaluations;  // (index, turns to empty)
        
        for (int idx : playableIndices) {
            // Create a new hand without this card
            std::vector<Card> newHand;
            for (int i = 0; i < static_cast<int>(hand.size()); i++) {
                if (i != idx) {
                    newHand.push_back(hand[i]);
                }
            }
            
            // Create new table layout with this card played
            auto newTableLayout = tableLayout;
            newTableLayout[hand[idx].suit][hand[idx].rank] = true;
            
            // Calculate minimum turns to empty
            int turnsToEmpty = calculateMinTurnsToEmpty(newHand, newTableLayout);
            
            moveEvaluations.push_back({idx, turnsToEmpty});
        }
        
        // Sort by turns to empty (ascending, with -1 last)
        std::sort(moveEvaluations.begin(), moveEvaluations.end(), 
                 [](const auto& a, const auto& b) {
                     if (a.second == -1) return false;
                     if (b.second == -1) return true;
                     return a.second < b.second;
                 });
        
        // If any move can lead to emptying the hand, choose the quickest
        if (!moveEvaluations.empty() && moveEvaluations[0].second != -1) {
            return moveEvaluations[0].first;
        }
        
        // If no path to empty hand was found, fall back to basic strategy
        // Prioritize cards that enable more future plays
        int bestIdx = playableIndices[0];
        int bestFuturePlays = -1;
        
        for (int idx : playableIndices) {
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            if (futurePlays > bestFuturePlays) {
                bestFuturePlays = futurePlays;
                bestIdx = idx;
            }
        }
        
        return bestIdx;
    }
    
    // Select the best move for early/mid game
    int selectEarlyMidGameMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        int gamePhase) const 
    {
        // Score each playable card
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = 1.0;  // Base score
            const Card& card = hand[idx];
            
            // Core endgame strategy focus: eliminate extreme cards early
            if (isExtremeCard(card)) {
                score += 2.0;  // Strong bonus for getting rid of extreme cards
            }
            
            // Future play potential is critical
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            score += 1.0 * futurePlays;
            
            // Prioritize singleton elimination differently based on game phase
            if (isSingleton(card, hand)) {
                // In early game, getting rid of singletons is good
                if (gamePhase == 0) {
                    score += 1.5;
                }
                // In mid game, be more cautious about singletons - check future plays
                else if (gamePhase == 1) {
                    if (futurePlays > 0) {
                        score += 1.0;  // Only eliminate if it doesn't leave us stuck
                    } else {
                        score -= 0.5;  // Slight penalty if it might leave us stuck
                    }
                }
            }
            
            // Avoid leaving ourselves with no future plays
            if (futurePlays == 0 && hand.size() > 1) {
                score -= 2.0;  // Strong penalty
            }
            
            // In mid-game, start planning ahead more
            if (gamePhase == 1) {
                // Create a new hand without this card
                std::vector<Card> newHand;
                for (int i = 0; i < static_cast<int>(hand.size()); i++) {
                    if (i != idx) {
                        newHand.push_back(hand[i]);
                    }
                }
                
                // Create new table layout with this card played
                auto newTableLayout = tableLayout;
                newTableLayout[hand[idx].suit][hand[idx].rank] = true;
                
                // Look ahead 1 level - how many cards would be playable after this move?
                int level2Playable = 0;
                for (int i = 0; i < static_cast<int>(newHand.size()); i++) {
                    if (isCardPlayable(newHand[i], newTableLayout)) {
                        level2Playable++;
                    }
                }
                
                // Bonus for moves that enable multiple future plays
                score += 0.5 * level2Playable;
            }
            
            // Special handling for 7s: prefer those in suits where we have cards
            if (card.rank == 7) {
                // Count cards in this suit
                int suitCount = 0;
                for (const Card& c : hand) {
                    if (c.suit == card.suit) {
                        suitCount++;
                    }
                }
                
                // Bonus for 7s in suits where we have more cards
                score += 0.3 * (suitCount - 1);  // -1 because the 7 itself is counted
            }
            
            // Add small random factor to break ties
            score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
            
            scoredMoves.push_back({idx, score});
        }
        
        // Sort by score (highest first)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return the highest-scoring move
        return scoredMoves[0].first;
    }
};

} // namespace sevens

#ifdef BUILDING_DLL
extern "C" {
    // Export function for dynamic library loading - works on both Windows and Linux
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    sevens::PlayerStrategy* createStrategy() {
        return new sevens::EndgameStrategy();
    }
}
#endif