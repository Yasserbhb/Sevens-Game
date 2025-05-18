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
 * SequenceStrategy: Focuses on maximizing the playing of consecutive cards in the same suit.
 * 
 * The core approach is to identify potential sequences in hand and prioritize
 * plays that enable chained shedding of multiple cards.
 */
class SequenceStrategy : public PlayerStrategy {
public:
    SequenceStrategy() {
        // Initialize RNG with time-based seed
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    ~SequenceStrategy() override = default;
    
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
        
        // Always play a 7 on the first turn if available
        if (roundTurn == 1) {
            for (int idx : playableIndices) {
                if (hand[idx].rank == 7) {
                    return idx;
                }
            }
        }
        
        // Calculate suit counts for decision making
        auto suitCounts = countCardsBySuit(hand);
        
        // Score each playable card
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = scoreMove(idx, hand, tableLayout, suitCounts);
            scoredMoves.push_back({idx, score});
        }
        
        // Sort by score (highest first)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return the highest-scoring move
        return scoredMoves[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // We don't need to track much in this strategy
        (void)playerID;
        (void)playedCard;
    }
    
    void observePass(uint64_t playerID) override {
        // We don't need to track passes in this strategy
        (void)playerID;
    }
    
    std::string getName() const override {
        return "SequenceStrategy";
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
            
            // Check if this card would be playable after we play our card
            bool wouldBePlayable = false;
            
            // If it's adjacent to our card or to another card already on the table
            if (r == rank - 1) {
                wouldBePlayable = true; // Adjacent to the card we're playing
            } else {
                // Check if r+1 would be on the table
                wouldBePlayable = simulatedTable.count(suit) > 0 && 
                                  simulatedTable.at(suit).count(r+1) > 0 && 
                                  simulatedTable.at(suit).at(r+1);
            }
            
            if (!wouldBePlayable) break;
            
            // Update the simulated table to include this card
            simulatedTable[suit][r] = true;
            length++;
        }
        
        // Reset simulated table to just our played card
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
            
            // Check if this card would be playable after we play our card
            bool wouldBePlayable = false;
            
            // If it's adjacent to our card or to another card already on the table
            if (r == rank + 1) {
                wouldBePlayable = true; // Adjacent to the card we're playing
            } else {
                // Check if r-1 would be on the table
                wouldBePlayable = simulatedTable.count(suit) > 0 && 
                                  simulatedTable.at(suit).count(r-1) > 0 && 
                                  simulatedTable.at(suit).at(r-1);
            }
            
            if (!wouldBePlayable) break;
            
            // Update the simulated table to include this card
            simulatedTable[suit][r] = true;
            length++;
        }
        
        return length;
    }
    
    // Score a potential move based on sequence building
    double scoreMove(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        const std::array<int, 4>& suitCounts) const 
    {
        const Card& card = hand[cardIdx];
        double score = 1.0; // Base score
        
        // Calculate the potential sequence length (this is the core of this strategy)
        int seqLength = calculateSequenceLength(cardIdx, hand, tableLayout);
        
        // Heavily weight sequence length - this is the primary focus
        score += 2.0 * seqLength;
        
        // For 7s, consider the number of cards in that suit
        if (card.rank == 7) {
            // Play 7s only in suits where we have many cards
            if (suitCounts[card.suit] >= 3) {
                score += 1.5;
            } else {
                // Penalize playing a 7 in a suit where we have few cards
                score -= 0.5;
            }
        }
        
        // Small bonus for extremes (A, K, Q, J, 2, 3) as they're harder to play
        if (card.rank <= 3 || card.rank >= 11) {
            score += 0.5;
        }
        
        // Small random factor to break ties
        score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
        
        return score;
    }
};

} // namespace sevens

#ifdef BUILD_SHARED_LIB
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::SequenceStrategy();
}
#endif