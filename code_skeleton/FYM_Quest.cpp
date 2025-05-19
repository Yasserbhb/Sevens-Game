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
 * FYM_Quest: A refined strategy that builds on SequenceStrategy's strengths
 * while incorporating adaptivity elements from BalanceStrategy and defensive aspects
 * from BlockingStrategy. This strategy performs enhanced sequence analysis and adapts
 * its approach based on player count, game phase, and board state.
 */
class FYM_Quest : public PlayerStrategy {
public:
    FYM_Quest() {
        // Initialize RNG with time-based seed
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
        
        // Initialize suit tracking
        for (int i = 0; i < 4; i++) {
            sevenStatus[i] = 0;
            suitCounts[i] = 0;
            suitImbalance[i] = 0.0;
        }
    }
    
    virtual ~FYM_Quest() = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        playerCount = 0;
        
        // Reset suit tracking
        for (int i = 0; i < 4; i++) {
            sevenStatus[i] = 0;
            suitCounts[i] = 0;
            suitImbalance[i] = 0.0;
        }
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Increment turn counter
        roundTurn++;
        
        // Update our analysis of the game state
        updateGameState(hand, tableLayout);
        
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
        
        // Estimate current player count if unknown
        if (playerCount == 0) {
            // Default estimate based on hand size
            playerCount = estimatePlayerCount(hand.size());
        }
        
        // Determine game phase
        int gamePhase = getGamePhase(hand);
        
        // Score each playable card based on our enhanced strategy
        std::vector<std::pair<int, double>> scoredMoves;
        for (int idx : playableIndices) {
            double score = scoreMoveEnhanced(idx, hand, tableLayout, gamePhase);
            scoredMoves.push_back({idx, score});
        }
        
        // Sort by score (highest first)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return the highest-scoring move
        return scoredMoves[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // Update seven status tracking
        if (playedCard.rank == 7) {
            sevenStatus[playedCard.suit] = -1; // 7 is played
        }
        
        // Update player count estimation
        if (playerID >= playerCount) {
            playerCount = playerID + 1;
        }
        
        // Reset consecutive passes since someone played
        consecutivePasses = 0;
    }
    
    void observePass(uint64_t playerID) override {
        // Track passes for game state analysis
        consecutivePasses++;
        
        // Update player count estimation
        if (playerID >= playerCount) {
            playerCount = playerID + 1;
        }
    }
    
    std::string getName() const override {
        return "FYM_Quest";
    }
    
private:
    // Game state tracking
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    int playerCount = 0;
    int consecutivePasses = 0;
    
    // Strategy weighting constants - tuned based on test results
    static constexpr double SEQUENCE_WEIGHT = 2.5;    // Primary focus on sequences
    static constexpr double SEVEN_WEIGHT = 1.5;       // Moderate weight for 7s
    static constexpr double BALANCE_WEIGHT = 1.2;     // Some consideration for suit balance
    static constexpr double BLOCKING_WEIGHT = 0.8;    // Minor consideration for blocking
    static constexpr double EXTREMES_WEIGHT = 1.7;    // Good weight for extreme cards in late game
    
    // Tracking for each suit
    std::array<int, 4> sevenStatus = {0, 0, 0, 0};  // 0=unknown, 1=in hand, -1=played
    std::array<int, 4> suitCounts = {0, 0, 0, 0};   // Cards per suit in hand
    std::array<double, 4> suitImbalance = {0.0, 0.0, 0.0, 0.0};  // Imbalance measure
    
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
    
    // Update our analysis of the game state
    void updateGameState(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        // Update seven status
        for (int suit = 0; suit < 4; suit++) {
            // Check if 7 is on the table
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(7) > 0 && 
                tableLayout.at(suit).at(7)) {
                sevenStatus[suit] = -1; // 7 is played
                continue;
            }
            
            // Check if we have the 7 in our hand
            bool have7 = false;
            for (const Card& card : hand) {
                if (card.suit == suit && card.rank == 7) {
                    have7 = true;
                    break;
                }
            }
            
            if (have7) {
                sevenStatus[suit] = 1; // 7 is in our hand
            } else if (sevenStatus[suit] != -1) {
                sevenStatus[suit] = 0; // 7 is unknown/not played
            }
        }
        
        // Update suit counts
        for (int suit = 0; suit < 4; suit++) {
            suitCounts[suit] = 0;
        }
        
        for (const Card& card : hand) {
            suitCounts[card.suit]++;
        }
        
        // Calculate suit imbalance
        calculateSuitImbalance(hand);
    }
    
    // Calculate imbalance for each suit relative to ideal distribution
    void calculateSuitImbalance(const std::vector<Card>& hand) {
        if (hand.empty()) return;
        
        // Calculate ideal count per suit
        double idealCount = static_cast<double>(hand.size()) / 4.0;
        
        // Calculate imbalance for each suit
        for (int suit = 0; suit < 4; suit++) {
            suitImbalance[suit] = static_cast<double>(suitCounts[suit]) - idealCount;
        }
    }
    
    // Estimate player count based on hand size
    int estimatePlayerCount(int handSize) const {
        // Simple heuristic based on typical starting hands
        if (handSize >= 12) return 4;       // 4 or fewer players
        else if (handSize >= 9) return 5;   // 5-6 players
        else if (handSize >= 7) return 7;   // 7-8 players
        else return 10;                     // 9+ players
    }
    
    // Determine game phase based on hand size and round turn
    int getGamePhase(const std::vector<Card>& hand) const {
        // Primary factor is hand size
        if (hand.size() > 10) return 0;     // Early game
        if (hand.size() > 5) return 1;      // Mid game
        if (hand.size() > 2) return 2;      // Late game
        return 3;                           // End game (≤ 2 cards)
    }
    
    // Enhanced sequence analysis - key improvement over original SequenceStrategy
    int analyzeSequence(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        int suit = card.suit;
        int rank = card.rank;
        
        // Track potential sequence length and played cards in simulation
        int maxLength = 1; // Start with the card itself
        std::vector<bool> simulatedPlayed(hand.size(), false);
        simulatedPlayed[cardIdx] = true;
        
        // Create a simulated table with this card played
        auto simulatedTable = tableLayout;
        simulatedTable[suit][rank] = true;
        
        // Map to track which cards in our hand would be playable
        std::vector<bool> wouldBePlayable(hand.size(), false);
        
        // Initial check for directly playable cards after this move
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (i != cardIdx && !simulatedPlayed[i] && 
                isCardPlayable(hand[i], simulatedTable)) {
                wouldBePlayable[i] = true;
            }
        }
        
        // Maximum length of a contiguous sequence we might play
        int currentLength = 1;
        bool foundPlayable;
        
        // Simulate playing cards in sequence until no more cards can be played
        do {
            foundPlayable = false;
            
            // Check if any card is playable in this step
            for (int i = 0; i < static_cast<int>(hand.size()); i++) {
                if (!simulatedPlayed[i] && wouldBePlayable[i]) {
                    // Found a playable card, simulate playing it
                    simulatedPlayed[i] = true;
                    currentLength++;
                    
                    // Update the simulated table
                    simulatedTable[hand[i].suit][hand[i].rank] = true;
                    
                    // This card is no longer playable (already played)
                    wouldBePlayable[i] = false;
                    
                    // Check for newly playable cards
                    for (int j = 0; j < static_cast<int>(hand.size()); j++) {
                        if (!simulatedPlayed[j] && !wouldBePlayable[j] && 
                            isCardPlayable(hand[j], simulatedTable)) {
                            wouldBePlayable[j] = true;
                        }
                    }
                    
                    foundPlayable = true;
                    break; // Only simulate one play per step
                }
            }
            
            if (currentLength > maxLength) {
                maxLength = currentLength;
            }
            
        } while (foundPlayable);
        
        return maxLength;
    }
    
    // Check if a card is extreme (A, 2, 3 or J, Q, K)
    bool isExtremeCard(const Card& card) const {
        return card.rank <= 3 || card.rank >= 11;
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
    
    // Check if a card will potentially block opponents by not opening new endpoints
    bool hasBlockingPotential(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        int suit = card.suit;
        int rank = card.rank;
        
        // 7s always open new play opportunities, so no blocking potential
        if (rank == 7) return false;
        
        // For other cards, check if we're creating a new endpoint
        // If we're not, then we have blocking potential
        if (rank > 1 && rank < 13) {
            // Check if rank-1 is already on the table
            bool lowerAlreadyOnTable = (tableLayout.count(suit) > 0 && 
                                      tableLayout.at(suit).count(rank-1) > 0 && 
                                      tableLayout.at(suit).at(rank-1));
                                      
            // Check if rank+1 is already on the table
            bool higherAlreadyOnTable = (tableLayout.count(suit) > 0 && 
                                       tableLayout.at(suit).count(rank+1) > 0 && 
                                       tableLayout.at(suit).at(rank+1));
            
            // Check if we have the adjacent cards in our hand
            bool haveLowerInHand = false;
            bool haveHigherInHand = false;
            
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == rank - 1) {
                    haveLowerInHand = true;
                }
                if (c.suit == suit && c.rank == rank + 1) {
                    haveHigherInHand = true;
                }
            }
            
            // If playing this card won't create new endpoints for others, it has blocking potential
            bool createsLowerEndpoint = !lowerAlreadyOnTable && !haveLowerInHand && rank > 1;
            bool createsHigherEndpoint = !higherAlreadyOnTable && !haveHigherInHand && rank < 13;
            
            return !createsLowerEndpoint && !createsHigherEndpoint;
        }
        
        return false;
    }
    
    // Enhanced scoring function for choosing the best move
    double scoreMoveEnhanced(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        int gamePhase) const 
    {
        const Card& card = hand[cardIdx];
        double score = 1.0; // Base score
        
        // 1. Sequence analysis - primary scoring factor
        int seqLength = analyzeSequence(cardIdx, hand, tableLayout);
        score += SEQUENCE_WEIGHT * (seqLength - 1) * 0.5;
        
        // 2. Handle 7s with context-awareness
        if (card.rank == 7) {
            // Base score for 7s
            double sevenScore = SEVEN_WEIGHT;
            
            // Adjust based on player count and suit strength
            if (playerCount <= 2) {
                // In 1v1 games, be more aggressive with 7s regardless of suit distribution
                sevenScore += 0.5;
            } else if (playerCount >= 7) {
                // In many-player games, only play 7s in over-represented suits
                if (suitImbalance[card.suit] > 0) {
                    sevenScore += 0.5;
                } else {
                    sevenScore -= 0.5;
                }
            } else {
                // In medium player games, be moderately selective
                if (suitCounts[card.suit] >= 3) {
                    sevenScore += 0.3;
                } else if (suitCounts[card.suit] <= 1) {
                    sevenScore -= 0.3;
                }
            }
            
            score += sevenScore;
        }
        
        // 3. Suit balance considerations
        if (playerCount >= 4) { // Only apply balance logic in multiplayer games
            // Bonus for playing from overrepresented suits
            if (suitImbalance[card.suit] > 0) {
                score += BALANCE_WEIGHT * suitImbalance[card.suit] * 0.4;
            }
        }
        
        // 4. Future play opportunities
        int futurePlays = countFuturePlays(cardIdx, hand, tableLayout);
        
        // Adjust weight based on game phase
        double futureFactor = 0.3;
        if (gamePhase >= 2) futureFactor = 0.5; // More important in late game
        
        score += (gamePhase == 3 ? 2.0 : 1.0) * futurePlays * futureFactor;
        
        // 5. Blocking potential in multiplayer games
        if (playerCount >= 4 && hasBlockingPotential(cardIdx, hand, tableLayout)) {
            score += BLOCKING_WEIGHT;
        }
        
        // 6. Extreme card handling (A, 2, 3, J, Q, K)
        if (isExtremeCard(card)) {
            // Apply increasing bonus for extreme cards as game progresses
            double extremeBonus = EXTREMES_WEIGHT * gamePhase * 0.3;
            score += extremeBonus;
        }
        
        // 7. Critical late-game logic
        if (gamePhase >= 2) {
            // Strong penalty for moves that leave no future options
            if (futurePlays == 0 && hand.size() > 1) {
                score -= 4.0;
            }
            
            // Bonus for emptying hand quickly
            score += 0.3 * (4 - gamePhase);
        }
        
        // 8. Small random factor to break ties and add unpredictability
        score += std::uniform_real_distribution<>(0.0, 0.08)(const_cast<std::mt19937&>(rng));
        
        return score;
    }
};

} // namespace sevens

#ifdef BUILD_SHARED_LIB
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::FYM_Quest();
}
#endif