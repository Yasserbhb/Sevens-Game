#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <chrono>
#include <string>

namespace sevens {

/**
 * SevensRushStrategy: Focuses on opening as many suits as possible by aggressively 
 * playing 7s at the first opportunity, maximizing the board state options.
 */
class SevensRushStrategy : public PlayerStrategy {
public:
    SevensRushStrategy() {
        // Initialize RNG with time-based seed
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    ~SevensRushStrategy() override = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
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
        
        // Core strategy: Always prioritize 7s
        // Check if we have any 7s that can be played - play them first!
        for (int idx : playableIndices) {
            if (hand[idx].rank == 7) {
                return idx; // Always play 7s immediately
            }
        }
        
        // No 7s available, now score other playable cards
        int gamePhase = getGamePhase(hand);
        
        // For late game, focus on emptying the hand efficiently
        if (gamePhase == 2) {
            return scoreLateGameMoves(playableIndices, hand, tableLayout);
        }
        
        // For early and mid game, apply standard scoring
        return scoreStandardMoves(playableIndices, hand, tableLayout);
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // This strategy doesn't require much opponent tracking
        (void)playerID;
        (void)playedCard;
    }
    
    void observePass(uint64_t playerID) override {
        // This strategy doesn't require much opponent tracking
        (void)playerID;
    }
    
    std::string getName() const override {
        return "SevensRushStrategy";
    }
    
private:
    // Game state tracking
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    
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
    
    // Special scoring for late game moves (5 or fewer cards)
    int scoreLateGameMoves(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        int bestIdx = playableIndices[0];
        double bestScore = -1000.0;
        
        for (int idx : playableIndices) {
            double score = 1.0; // Base score
            const Card& card = hand[idx];
            
            // In late game, prioritize singletons and extreme cards
            if (isSingleton(card, hand)) {
                score += 2.0; // Strong preference for singletons
            }
            
            // Bonus for extreme ranks (A,2,3 and J,Q,K) as they're harder to play
            if (card.rank <= 3 || card.rank >= 11) {
                score += 1.5;
            }
            
            // Look ahead to future playability
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            if (hand.size() <= 3 && futurePlays > 0) {
                score += 3.0; // Very important to have future plays with few cards
            } else {
                score += 0.5 * futurePlays; // Still good to have options
            }
            
            // Add small random factor to break ties
            score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
            
            if (score > bestScore) {
                bestScore = score;
                bestIdx = idx;
            }
        }
        
        return bestIdx;
    }
    
    // Standard scoring for early and mid game moves
    int scoreStandardMoves(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        int bestIdx = playableIndices[0];
        double bestScore = -1000.0;
        
        for (int idx : playableIndices) {
            double score = 1.0; // Base score
            const Card& card = hand[idx];
            
            // Count cards in the same suit
            int suitCount = 0;
            for (const Card& c : hand) {
                if (c.suit == card.suit) {
                    suitCount++;
                }
            }
            
            // Moderately prefer cards that are close to 7
            int distanceFrom7 = std::abs(card.rank - 7);
            score += 0.2 * (7 - distanceFrom7); // Higher for cards closer to 7
            
            // Small bonus for suits where we have more cards
            score += 0.1 * suitCount;
            
            // Small bonus for singletons even in early/mid game
            if (suitCount == 1) {
                score += 0.7;
            }
            
            // Look ahead to future playability
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            score += 0.3 * futurePlays;
            
            // Add small random factor to break ties
            score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
            
            if (score > bestScore) {
                bestScore = score;
                bestIdx = idx;
            }
        }
        
        return bestIdx;
    }
};

} // namespace sevens

#ifdef BUILD_SHARED_LIB
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::SevensRushStrategy();
}
#endif