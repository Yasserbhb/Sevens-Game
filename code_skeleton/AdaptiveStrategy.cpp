#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <chrono>
#include <string>
#include <deque>
#include <set>

namespace sevens {

/**
 * AdaptiveStrategy: Dynamically shifts strategy based on game state and opponent behavior.
 * It balances between aggression and defense, adjusting weights based on observations.
 */
class AdaptiveStrategy : public PlayerStrategy {
public:
    AdaptiveStrategy() {
        // Initialize RNG with time-based seed
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
        
        // Initialize with balanced weights
        resetStrategyWeights();
    }
    
    virtual ~AdaptiveStrategy() = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        totalMoves = 0;
        totalPasses = 0;
        
        // Reset strategy weights to default
        resetStrategyWeights();
        
        // Clear tracking data
        recentPasses.clear();
        lastObservedMoves.clear();
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Increment turn counter
        roundTurn++;
        
        // Update our tracking of 7s
        updateSevenStatus(hand, tableLayout);
        
        // Update strategy weights based on game state and opponent behavior
        updateStrategyWeights();
        
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
        
        // Score each playable card using our adaptive weights
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = scoreMove(idx, hand, tableLayout);
            scoredMoves.push_back({idx, score});
        }
        
        // Sort by score (highest first)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return the highest-scoring move
        return scoredMoves[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        if (playerID == myID) return; // Don't track our own moves
        
        // Record this move
        lastObservedMoves.push_front({playerID, playedCard});
        if (lastObservedMoves.size() > MAX_MOVE_HISTORY) {
            lastObservedMoves.pop_back();
        }
        
        // Track when 7s are played
        if (playedCard.rank == 7) {
            sevenStatus[playedCard.suit] = -1; // Mark as played
        }
        
        // Keep track of active players
        activePlayers.insert(playerID);
        
        // Remove this player from recent passes
        recentPasses.erase(playerID);
        
        // Update total moves count
        totalMoves++;
    }
    
    void observePass(uint64_t playerID) override {
        if (playerID == myID) return; // Don't track our own passes
        
        // Record this pass
        recentPasses.insert(playerID);
        
        // Keep track of active players
        activePlayers.insert(playerID);
        
        // Update total passes count
        totalPasses++;
    }
    
    std::string getName() const override {
        return "AdaptiveStrategy";
    }
    
private:
    // Constants
    static constexpr int MAX_MOVE_HISTORY = 20;
    
    // Game state tracking
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    int totalMoves = 0;
    int totalPasses = 0;
    
    // Strategy weights that will be dynamically adjusted
    double sevenWeight = 1.5;       // Weight for playing 7s
    double blockingWeight = 1.0;    // Weight for blocking opponents
    double sequenceWeight = 1.2;    // Weight for sequence building
    double singletonWeight = 1.0;   // Weight for singleton elimination
    double futurePlayWeight = 1.0;  // Weight for future play opportunities
    
    // Tracks the status of 7s for each suit
    // 1 = in our hand, -1 = played on table, 0 = unknown/not played
    std::array<int, 4> sevenStatus = {0, 0, 0, 0};
    
    // Opponent tracking
    std::set<uint64_t> recentPasses;  // Players who passed most recently
    std::set<uint64_t> activePlayers; // All players observed
    
    // Recent move history to analyze patterns
    std::deque<std::pair<uint64_t, Card>> lastObservedMoves;
    
    // Reset strategy weights to balanced default values
    void resetStrategyWeights() {
        sevenWeight = 1.5;
        blockingWeight = 1.0;
        sequenceWeight = 1.2;
        singletonWeight = 1.0;
        futurePlayWeight = 1.0;
    }
    
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
    
    // Update our knowledge of which 7s are on the table and in our hand
    void updateSevenStatus(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        // Check each suit
        for (int suit = 0; suit < 4; suit++) {
            // Is the 7 of this suit on the table?
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(7) > 0 && 
                tableLayout.at(suit).at(7)) {
                sevenStatus[suit] = -1; // 7 is on table
                continue;
            }
            
            // Do we have the 7 of this suit in hand?
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
                sevenStatus[suit] = 0; // 7 is neither on table nor in our hand
            }
        }
    }
    
    // Count cards by suit in hand
    std::array<int, 4> countCardsBySuit(const std::vector<Card>& hand) const {
        std::array<int, 4> counts = {0, 0, 0, 0};
        
        for (const Card& card : hand) {
            counts[card.suit]++;
        }
        
        return counts;
    }
    
    // Calculate sequence length potential for a card
    int calculateSequenceLength(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        int suit = card.suit;
        int rank = card.rank;
        int length = 1; // Start with the card itself
        
        // Create a simulated table with this card played
        auto simulatedTable = tableLayout;
        simulatedTable[suit][rank] = true;
        
        // Check for cards we have in sequence below this card
        for (int r = rank - 1; r >= 1; r--) {
            bool haveCardInHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == r) {
                    haveCardInHand = true;
                    break;
                }
            }
            
            if (!haveCardInHand) break;
            
            // Check if this card would be playable
            if (r == rank - 1) {
                // Directly adjacent to our card - would be playable
                length++;
                simulatedTable[suit][r] = true; // Update simulated table
            } else {
                // Need to check if r+1 would be on the table
                if (simulatedTable.count(suit) > 0 && 
                    simulatedTable.at(suit).count(r+1) > 0 && 
                    simulatedTable.at(suit).at(r+1)) {
                    length++;
                    simulatedTable[suit][r] = true; // Update simulated table
                } else {
                    break;
                }
            }
        }
        
        // Reset simulated table
        simulatedTable = tableLayout;
        simulatedTable[suit][rank] = true;
        
        // Check for cards we have in sequence above this card
        for (int r = rank + 1; r <= 13; r++) {
            bool haveCardInHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == r) {
                    haveCardInHand = true;
                    break;
                }
            }
            
            if (!haveCardInHand) break;
            
            // Check if this card would be playable
            if (r == rank + 1) {
                // Directly adjacent to our card - would be playable
                length++;
                simulatedTable[suit][r] = true; // Update simulated table
            } else {
                // Need to check if r-1 would be on the table
                if (simulatedTable.count(suit) > 0 && 
                    simulatedTable.at(suit).count(r-1) > 0 && 
                    simulatedTable.at(suit).at(r-1)) {
                    length++;
                    simulatedTable[suit][r] = true; // Update simulated table
                } else {
                    break;
                }
            }
        }
        
        return length;
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
    
    // Check if playing this card would likely enable opponent plays
    bool willEnableOpponentPlays(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        int suit = card.suit;
        int rank = card.rank;
        
        // 7s always enable plays on both sides
        if (rank == 7) {
            return true;
        }
        
        // For non-7 cards, check if we're creating a new endpoint
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
            
            // If this card creates a new endpoint that we don't have the adjacent card for,
            // it may enable opponent plays
            if ((!lowerAlreadyOnTable && !haveLowerInHand && rank > 1) || 
                (!higherAlreadyOnTable && !haveHigherInHand && rank < 13)) {
                return true;
            }
        }
        
        return false;
    }
    
    // Get game phase based on hand size
    int getGamePhase(const std::vector<Card>& hand) const {
        if (hand.size() > 10) return 0; // Early game
        if (hand.size() > 5) return 1;  // Mid game
        return 2;                       // Late game
    }
    
    // Update strategy weights based on game state and opponent behavior
    void updateStrategyWeights() {
        // Calculate pass rate
        double passRate = 0.0;
        if (totalMoves + totalPasses > 0) {
            passRate = static_cast<double>(totalPasses) / (totalMoves + totalPasses);
        }
        
        // Number of times 7s have been played recently
        int recent7sPlayed = 0;
        for (const auto& move : lastObservedMoves) {
            if (move.second.rank == 7) {
                recent7sPlayed++;
            }
        }
        
        // Adjust weights based on observations
        
        // 1. If opponents pass a lot, be more aggressive with 7s
        if (passRate > 0.4) {
            sevenWeight = 2.5;
            blockingWeight = 0.8; // Less focus on blocking
        } else {
            sevenWeight = 1.2;
            blockingWeight = 1.8; // More focus on blocking
        }
        
        // 2. If many 7s are being played, adjust accordingly
        if (recent7sPlayed > 5) {
            // If 7s are being played aggressively, focus more on sequence building
            sequenceWeight = 2.0;
            sevenWeight *= 0.8; // Reduce 7s weight as many are likely already played
        }
        
        // 3. Adjust singleton weight based on number of active players
        int playerCount = activePlayers.size();
        if (playerCount > 3) {
            // In games with many players, singletons become more valuable
            singletonWeight = 1.5;
        } else {
            singletonWeight = 1.0;
        }
        
        // 4. Adjust future play weight based on game phase
        if (roundTurn < 5) {
            // Early in the round, future plays are more important
            futurePlayWeight = 1.5;
        } else {
            futurePlayWeight = 1.0;
        }
        
        // 5. Randomize weights slightly to add unpredictability
        sevenWeight += std::uniform_real_distribution<>(-0.1, 0.1)(rng);
        blockingWeight += std::uniform_real_distribution<>(-0.1, 0.1)(rng);
        sequenceWeight += std::uniform_real_distribution<>(-0.1, 0.1)(rng);
        singletonWeight += std::uniform_real_distribution<>(-0.1, 0.1)(rng);
        futurePlayWeight += std::uniform_real_distribution<>(-0.1, 0.1)(rng);
    }
    
    // Score a potential move using adaptive weights
    double scoreMove(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        double score = 1.0; // Base score
        int gamePhase = getGamePhase(hand);
        auto suitCounts = countCardsBySuit(hand);
        
        // 1. Seven scoring - adjusted by dynamic seven weight
        if (card.rank == 7) {
            double sevenScore = sevenWeight;
            
            // Adjust based on suit strength
            if (suitCounts[card.suit] >= 3) {
                sevenScore += 0.5;
            } else if (suitCounts[card.suit] == 1) {
                sevenScore -= 0.5; // Penalty for opening a suit where we just have the 7
            }
            
            score += sevenScore;
        }
        
        // 2. Blocking consideration - adjusted by dynamic blocking weight
        if (!willEnableOpponentPlays(cardIdx, hand, tableLayout)) {
            score += blockingWeight;
        }
        
        // 3. Sequence building - adjusted by dynamic sequence weight
        int seqLength = calculateSequenceLength(cardIdx, hand, tableLayout);
        if (seqLength > 1) {
            score += sequenceWeight * (seqLength - 1) * 0.5;
        }
        
        // 4. Singleton management - adjusted by dynamic singleton weight
        if (isSingleton(card, hand)) {
            // Higher bonus in late game
            score += singletonWeight * (1.0 + 0.3 * gamePhase);
        }
        
        // 5. Future play opportunities - adjusted by dynamic future play weight
        int futurePlays = countFuturePlays(cardIdx, hand, tableLayout);
        score += futurePlayWeight * futurePlays * 0.3;
        
        // 6. Game phase specific adjustments
        if (gamePhase == 2) { // Late game
            // In late game, prioritize extreme cards and emptying hand
            if (card.rank <= 3 || card.rank >= 11) {
                score += 1.0;
            }
            
            // Ensure we can play again if possible
            if (futurePlays == 0 && hand.size() > 1) {
                score -= 2.0; // Significant penalty
            }
        }
        
        // 7. Small random factor to break ties
        score += std::uniform_real_distribution<>(0.0, 0.05)(const_cast<std::mt19937&>(rng));
        
        return score;
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
        return new sevens::AdaptiveStrategy();
    }
}
#endif