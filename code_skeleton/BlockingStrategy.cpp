#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <chrono>
#include <string>
#include <set>

namespace sevens {

/**
 * BlockingStrategy: Focuses on preventing opponents from playing by strategically
 * withholding key cards, especially 7s in suits where we have few cards.
 */
class BlockingStrategy : public PlayerStrategy {
public:
    BlockingStrategy() {
        // Initialize RNG with time-based seed
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
        
        // Initialize tracking arrays
        for (int i = 0; i < 4; i++) {
            sevenStatus[i] = 0;
            suitBlockValue[i] = 0;
        }
    }
    
    ~BlockingStrategy() override = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        consecutivePasses = 0;
        
        // Reset suit tracking
        for (int i = 0; i < 4; i++) {
            sevenStatus[i] = 0;
            suitBlockValue[i] = 0;
            playersPassing[i].clear();
        }
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Increment turn counter
        roundTurn++;
        
        // Update our tracking of 7s and suit counts
        updateSevenStatus(hand, tableLayout);
        auto suitCounts = countCardsBySuit(hand);
        
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
        
        // Exception for first turn - play the 7 of Diamonds if we have it
        if (roundTurn == 1) {
            for (int idx : playableIndices) {
                const Card& card = hand[idx];
                if (card.rank == 7 && card.suit == 1) { // 7 of Diamonds
                    return idx;
                }
            }
        }
        
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
        // If another player plays, reset consecutive passes
        if (playerID != myID) {
            consecutivePasses = 0;
        }
        
        // Track when 7s are played
        if (playedCard.rank == 7) {
            sevenStatus[playedCard.suit] = -1; // Mark as played
        }
        
        // Track the maximum player ID to estimate player count
        if (playerID >= maxPlayerID) {
            maxPlayerID = playerID + 1;
        }
        
        // Clear passing records for the suit that was just played
        playersPassing[playedCard.suit].clear();
    }
    
    void observePass(uint64_t playerID) override {
        // Don't track our own passes
        if (playerID == myID) {
            return;
        }
        
        // Increment consecutive passes if this isn't us
        consecutivePasses++;
        
        // Track which player passed
        if (playerID >= maxPlayerID) {
            maxPlayerID = playerID + 1;
        }
        
        // Track which suits players are likely unable to play in
        for (int suit = 0; suit < 4; suit++) {
            // For each suit that has cards on the table, assume the player can't play
            if (sevenStatus[suit] == -1) {
                playersPassing[suit].insert(playerID);
            }
        }
        
        // Update blocking values for each suit based on passes
        updateSuitBlockValues();
    }
    
    std::string getName() const override {
        return "BlockingStrategy";
    }
    
private:
    // Game state tracking
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    int consecutivePasses = 0;
    uint64_t maxPlayerID = 0;
    
    // Tracks the status of 7s for each suit
    // 1 = in our hand, -1 = played on table, 0 = unknown/not played
    std::array<int, 4> sevenStatus = {0, 0, 0, 0};
    
    // Block value for each suit - higher means more opponents are blocked in this suit
    std::array<int, 4> suitBlockValue = {0, 0, 0, 0};
    
    // Track which players seem to be passing on each suit
    std::array<std::set<uint64_t>, 4> playersPassing;
    
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
    
    // Update blocking values for each suit based on observed passes
    void updateSuitBlockValues() {
        for (int suit = 0; suit < 4; suit++) {
            // Count how many players seem blocked in this suit
            suitBlockValue[suit] = playersPassing[suit].size();
        }
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
        
        // 7s are special - they enable plays on both sides
        if (rank == 7) {
            return true;
        }
        
        // Create a simulated table with this card added
        auto simulatedTable = tableLayout;
        simulatedTable[suit][rank] = true;
        
        // Check if playing this card would open a new play opportunity
        
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
            
            // If this card creates a new endpoint in either direction, it may enable opponent plays
            if ((!lowerAlreadyOnTable && rank > 1) || (!higherAlreadyOnTable && rank < 13)) {
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
    
    // Score a potential move based on blocking strategy
    double scoreMove(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        const std::array<int, 4>& suitCounts) const 
    {
        // Mark cardIdx as used to prevent the compiler warning
        (void)cardIdx;
        
        const Card& card = hand[cardIdx];
        double score = 1.0; // Base score
        int gamePhase = getGamePhase(hand);
        
        // 1. Special handling for 7s - the core of the blocking strategy
        if (card.rank == 7) {
            // Generally avoid playing 7s unless we have many cards in that suit
            if (suitCounts[card.suit] >= 3) {
                score += 2.0; // Good to play 7s in suits we're strong in
            } else {
                // Strong penalty for playing 7s in suits we're weak in
                // This is the key blocking aspect - we hold these 7s to prevent others from playing
                score -= 3.0;
                
                // Even stronger penalty in early/mid game
                if (gamePhase < 2) {
                    score -= 1.0;
                }
            }
            
            // Exception: If this is the 7 of Diamonds at the start, we have to play it
            if (card.suit == 1 && tableLayout.empty()) {
                return 100.0; // Extremely high score to ensure we play it
            }
        }
        
        // 2. Blocking aspect - prefer cards that don't enable new opponent plays
        if (!willEnableOpponentPlays(cardIdx, hand, tableLayout)) {
            score += 2.0; // Big bonus for moves that don't help opponents
        }
        
        // 3. Consider the blocking value of this suit
        score += 0.5 * suitBlockValue[card.suit];
        
        // 4. In late game, shift focus to emptying hand
        if (gamePhase == 2) {
            // In endgame, care less about blocking and more about getting rid of cards
            score += 1.0;
            
            // Bonus for high/low cards that are harder to play
            if (card.rank <= 3 || card.rank >= 11) {
                score += 1.0;
            }
        }
        
        // 5. Small random factor to break ties
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
        return new sevens::BlockingStrategy();
    }
}
#endif