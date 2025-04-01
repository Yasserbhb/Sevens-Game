#include "PlayerStrategy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <random>
#include <chrono>
#include <cmath>
#include <numeric>

namespace sevens {

/**
 * An advanced strategy for the Sevens card game.
 * This strategy combines tracking, blocking, and adaptive play.
 */
class AdvancedSevensStrategy : public PlayerStrategy {
public:
    AdvancedSevensStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
        
        // Initialize card tracking
        for (int suit = 0; suit < 4; ++suit) {
            for (int rank = 1; rank <= 13; ++rank) {
                cardsPlayed[suit][rank] = false;
                
                // 7s are already on the table at game start
                if (rank == 7) {
                    cardsPlayed[suit][rank] = true;
                }
            }
        }
        
        // Initialize player estimates
        numPlayers = 1; // Will be updated when we observe other players
    }
    
    ~AdvancedSevensStrategy() override = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        
        // Reset game state
        for (int suit = 0; suit < 4; ++suit) {
            for (int rank = 1; rank <= 13; ++rank) {
                cardsPlayed[suit][rank] = (rank == 7); // Only 7s start on table
            }
        }
        
        movesWithoutPlay = 0;
        currentGameStage = GameStage::EARLY;
        totalPasses = 0;
        myHandSize = 0; // Will be updated when we first see our hand
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Update our internal table layout
        updateTableLayout(tableLayout);
        
        // First time we see our hand, record its size
        if (myHandSize == 0) {
            myHandSize = hand.size();
            // Estimate number of players based on hand size
            // Standard 52 card deck, minus 4 sevens on table = 48 cards
            numPlayers = 48 / myHandSize;
            if (numPlayers <= 1) numPlayers = 2; // Minimum safeguard
        }
        
        // Update our knowledge of what cards have been played
        myHandSize = hand.size();
        
        // Update game stage
        updateGameStage();
        
        // If no cards in hand, we're already done
        if (hand.empty()) {
            return -1;
        }
        
        // Find all playable cards
        std::vector<int> playableIndices;
        for (size_t i = 0; i < hand.size(); ++i) {
            if (isPlayable(hand[i], tableLayout)) {
                playableIndices.push_back(i);
            }
        }
        
        // If no playable cards, pass
        if (playableIndices.empty()) {
            totalPasses++;
            movesWithoutPlay++;
            return -1;
        }
        
        // Reset moves without play counter
        movesWithoutPlay = 0;
        
        // Depending on game stage, use different selection strategies
        switch (currentGameStage) {
            case GameStage::EARLY:
                return selectEarlyGameCard(hand, playableIndices);
            case GameStage::MID:
                return selectMidGameCard(hand, playableIndices);
            case GameStage::LATE:
                return selectLateGameCard(hand, playableIndices);
            default:
                return selectMidGameCard(hand, playableIndices);
        }
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // Track the card that was played
        cardsPlayed[playedCard.suit][playedCard.rank] = true;
        
        // Reset moves without play counter
        movesWithoutPlay = 0;
        
        // Track this player if we haven't seen them before
        if (playerID != myID && playerSeen.insert(playerID).second) {
            // Update our estimate of player count if needed
            if (playerSeen.size() + 1 > numPlayers) { // +1 for ourselves
                numPlayers = playerSeen.size() + 1;
            }
        }
    }
    
    void observePass(uint64_t playerID) override {
        // Count total passes
        totalPasses++;
        
        // Track this player if we haven't seen them before
        if (playerID != myID && playerSeen.insert(playerID).second) {
            // Update our estimate of player count if needed
            if (playerSeen.size() + 1 > numPlayers) { // +1 for ourselves
                numPlayers = playerSeen.size() + 1;
            }
        }
        
        // Increment moves without play counter
        movesWithoutPlay++;
    }
    
    std::string getName() const override {
        return "AdvancedSevensStrategy";
    }

private:
    uint64_t myID;
    std::mt19937 rng;
    
    // Card tracking - maps suit and rank to whether the card has been played
    std::map<int, std::map<int, bool>> cardsPlayed;
    
    // Player tracking
    std::set<uint64_t> playerSeen;
    uint64_t numPlayers;
    uint64_t myHandSize;
    
    // Game state tracking
    uint64_t totalPasses;
    uint64_t movesWithoutPlay;
    
    // Game stage enumeration
    enum class GameStage {
        EARLY,  // Start of game, focus on playing high-value cards
        MID,    // Mid-game, focus on blocking
        LATE    // End-game, focus on playing any card possible
    };
    
    GameStage currentGameStage;
    
    // Helper methods
    
    /**
     * Update our internal representation of the table layout
     */
    void updateTableLayout(const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        for (const auto& suitEntry : tableLayout) {
            uint64_t suit = suitEntry.first;
            for (const auto& rankEntry : suitEntry.second) {
                uint64_t rank = rankEntry.first;
                bool onTable = rankEntry.second;
                
                if (onTable) {
                    cardsPlayed[suit][rank] = true;
                }
            }
        }
    }
    
    /**
     * Check if a card is playable based on the current table layout
     */
    bool isPlayable(const Card& card, const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const {
        // Get the suit map
        auto suitIter = tableLayout.find(card.suit);
        if (suitIter == tableLayout.end()) {
            return false;
        }
        
        const auto& suitMap = suitIter->second;
        
        // Check if card is adjacent to a card already on table
        // Higher rank
        auto higherRankIter = suitMap.find(card.rank + 1);
        if (higherRankIter != suitMap.end() && higherRankIter->second) {
            return true;
        }
        
        // Lower rank
        auto lowerRankIter = suitMap.find(card.rank - 1);
        if (lowerRankIter != suitMap.end() && lowerRankIter->second) {
            return true;
        }
        
        return false;
    }
    
    /**
     * Calculate the blocking value of a card
     * Higher value = more blocking potential
     */
    double calculateBlockingValue(const Card& card) const {
        // Higher blocking value if this card might block other players
        double blockingValue = 0.0;
        
        // Check if cards adjacent to this one have been played
        bool lowerCardPlayed = (card.rank > 1) ? cardsPlayed.at(card.suit).at(card.rank - 1) : true;
        bool higherCardPlayed = (card.rank < 13) ? cardsPlayed.at(card.suit).at(card.rank + 1) : true;
        
        // If both adjacent cards are already on the table, this card has no blocking value
        if (lowerCardPlayed && higherCardPlayed) {
            return 0.0;
        }
        
        // Cards closer to the edges (A, K) have higher blocking potential
        double edgeFactor = 1.0;
        if (card.rank <= 3 || card.rank >= 11) {
            edgeFactor = 1.5;
        }
        
        // Cards that bridge a gap have higher blocking potential
        // For example: if 5 and 7 are on the table, the 6 bridges them
        bool twoLowerCardPlayed = (card.rank > 2) ? cardsPlayed.at(card.suit).at(card.rank - 2) : false;
        bool twoHigherCardPlayed = (card.rank < 12) ? cardsPlayed.at(card.suit).at(card.rank + 2) : false;
        
        double bridgeFactor = 1.0;
        if ((twoLowerCardPlayed && !lowerCardPlayed) || (twoHigherCardPlayed && !higherCardPlayed)) {
            bridgeFactor = 2.0;
        }
        
        // Combine factors
        blockingValue = edgeFactor * bridgeFactor;
        
        return blockingValue;
    }
    
    /**
     * Update the game stage based on current state
     */
    void updateGameStage() {
        // Estimate total cards in play (standard deck minus 7s already on table)
        const int totalCardsInPlay = 48; // 52 - 4 sevens
        
        // Estimate cards remaining based on observed plays and passes
        int estimatedCardsPlayed = 0;
        for (const auto& suitEntry : cardsPlayed) {
            for (const auto& rankEntry : suitEntry.second) {
                if (rankEntry.second && rankEntry.first != 7) { // Don't count initial 7s
                    estimatedCardsPlayed++;
                }
            }
        }
        
        // Calculate progress as a percentage
        double progress = static_cast<double>(estimatedCardsPlayed) / totalCardsInPlay;
        
        // Update game stage based on progress
        if (progress < 0.3) {
            currentGameStage = GameStage::EARLY;
        } else if (progress < 0.7) {
            currentGameStage = GameStage::MID;
        } else {
            currentGameStage = GameStage::LATE;
        }
        
        // Also consider number of passes
        if (totalPasses > numPlayers * 2) {
            // If we've seen a lot of passes, move to a later game stage
            if (currentGameStage == GameStage::EARLY) {
                currentGameStage = GameStage::MID;
            } else if (currentGameStage == GameStage::MID && totalPasses > numPlayers * 4) {
                currentGameStage = GameStage::LATE;
            }
        }
        
        // Also consider our hand size
        if (myHandSize < 3) {
            currentGameStage = GameStage::LATE;
        }
    }
    
    /**
     * Early game strategy: focus on playing cards that unblock others in our hand
     */
    int selectEarlyGameCard(const std::vector<Card>& hand, const std::vector<int>& playableIndices) {
        // In early game, we want to play cards that enable more future plays
        
        // For each playable card, count how many cards it would unblock in our hand
        std::vector<std::pair<int, int>> indexUnblockPairs;
        
        for (int index : playableIndices) {
            const Card& card = hand[index];
            int unblockCount = 0;
            
            // After playing this card, how many more cards would become playable?
            for (size_t i = 0; i < hand.size(); ++i) {
                if (i != index && std::find(playableIndices.begin(), playableIndices.end(), i) == playableIndices.end()) {
                    // If card is adjacent to the current card, it would become playable
                    if (hand[i].suit == card.suit && 
                        (hand[i].rank == card.rank + 1 || hand[i].rank == card.rank - 1)) {
                        unblockCount++;
                    }
                }
            }
            
            indexUnblockPairs.emplace_back(index, unblockCount);
        }
        
        // Sort by unblock count (descending)
        std::sort(indexUnblockPairs.begin(), indexUnblockPairs.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // If there's a tie for most unblocking, prefer cards further from 7
        if (indexUnblockPairs.size() > 1 && indexUnblockPairs[0].second == indexUnblockPairs[1].second) {
            int index1 = indexUnblockPairs[0].first;
            int index2 = indexUnblockPairs[1].first;
            
            int distFromSeven1 = std::abs(hand[index1].rank - 7);
            int distFromSeven2 = std::abs(hand[index2].rank - 7);
            
            if (distFromSeven1 > distFromSeven2) {
                return index1;
            } else if (distFromSeven2 > distFromSeven1) {
                return index2;
            }
        }
        
        // Return the card that unblocks the most others, or if none, the first playable
        if (!indexUnblockPairs.empty() && indexUnblockPairs[0].second > 0) {
            return indexUnblockPairs[0].first;
        }
        
        // If no card unblocks others, play high rank cards first
        std::vector<std::pair<int, int>> indexRankPairs;
        for (int index : playableIndices) {
            indexRankPairs.emplace_back(index, hand[index].rank);
        }
        
        // Sort by rank (descending)
        std::sort(indexRankPairs.begin(), indexRankPairs.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return indexRankPairs[0].first;
    }
    
    /**
     * Mid game strategy: focus on blocking opponents
     */
    int selectMidGameCard(const std::vector<Card>& hand, const std::vector<int>& playableIndices) {
        // In mid-game, we want to block opponents while enabling our future plays
        
        // Calculate blocking value for each playable card
        std::vector<std::tuple<int, double, double>> indexBlockingPairs; // (index, blocking value, enabling value)
        
        for (int index : playableIndices) {
            const Card& card = hand[index];
            double blockingValue = calculateBlockingValue(card);
            
            // Also calculate how many cards this would enable in our hand
            double enablingValue = 0.0;
            for (size_t i = 0; i < hand.size(); ++i) {
                if (i != index && std::find(playableIndices.begin(), playableIndices.end(), i) == playableIndices.end()) {
                    // If card is adjacent to the current card, it would become playable
                    if (hand[i].suit == card.suit && 
                        (hand[i].rank == card.rank + 1 || hand[i].rank == card.rank - 1)) {
                        enablingValue += 1.0;
                    }
                }
            }
            
            // Combine values (giving more weight to enabling our own plays)
            double combinedValue = blockingValue * 0.6 + enablingValue * 1.0;
            indexBlockingPairs.emplace_back(index, blockingValue, enablingValue);
        }
        
        // Sort by combined value (descending)
        std::sort(indexBlockingPairs.begin(), indexBlockingPairs.end(),
                 [](const auto& a, const auto& b) { 
                     double aValue = std::get<1>(a) * 0.6 + std::get<2>(a) * 1.0;
                     double bValue = std::get<1>(b) * 0.6 + std::get<2>(b) * 1.0;
                     return aValue > bValue; 
                 });
        
        // Return the card with the highest combined value
        return std::get<0>(indexBlockingPairs[0]);
    }
    
    /**
     * Late game strategy: focus on emptying hand as quickly as possible
     */
    int selectLateGameCard(const std::vector<Card>& hand, const std::vector<int>& playableIndices) {
        // In late game, just focus on playing cards and emptying hand
        
        // Count cards of each suit in hand
        std::map<int, int> suitCounts;
        for (const auto& card : hand) {
            suitCounts[card.suit]++;
        }
        
        // Find playable cards from suits we have the most of
        std::vector<std::pair<int, int>> indexSuitCountPairs;
        for (int index : playableIndices) {
            int suit = hand[index].suit;
            indexSuitCountPairs.emplace_back(index, suitCounts[suit]);
        }
        
        // Sort by suit count (descending)
        std::sort(indexSuitCountPairs.begin(), indexSuitCountPairs.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return the card from the most common suit
        return indexSuitCountPairs[0].first;
    }
};

extern "C" PlayerStrategy* createStrategy() {
    return new AdvancedSevensStrategy();
}

} // namespace sevens