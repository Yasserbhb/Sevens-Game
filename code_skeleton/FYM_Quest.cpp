#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <chrono>
#include <iostream>
#include <string>
#include <cmath>
#include <numeric>
#include <unordered_set>

namespace sevens {

/**
 * An advanced strategy for the Sevens card game that implements sophisticated
 * decision-making algorithms to maximize win rate.
 */
class FYM_Quest : public PlayerStrategy {
public:
    FYM_Quest() {
        // Seed the RNG with the current time
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
        
        // Initialize game state tracking variables
        roundTurn = 0;
        gamePhase = 0;
        numPlayers = 4; // Default assumption, will be updated as we observe game
        passesSinceMyLastPlay = 0;
        
        // Initialize player hand size tracking
        for (int i = 0; i < MAX_PLAYERS; i++) {
            playerHandSizes[i] = -1; // -1 indicates unknown
        }
        
        // Initialize suit counts
        for (int i = 0; i < 4; i++) {
            suitCounts[i] = 0;
            sevenPlayability[i] = 0; // 0 = unknown
        }
    }
    
    ~FYM_Quest() override = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        gamePhase = 0;
        passesSinceMyLastPlay = 0;
        playedCards.clear();
        
        // Set all hand sizes to default (assuming 52 cards distributed evenly)
        // We'll update this as we observe the game
        int assumedHandSize = 13; // 52/4, assuming 4 players
        for (int i = 0; i < MAX_PLAYERS; i++) {
            playerHandSizes[i] = (i == static_cast<int>(myID)) ? -1 : assumedHandSize;
        }
        
        // Reset suit-related trackers
        for (int i = 0; i < 4; i++) {
            suitCounts[i] = 0;
            sevenPlayability[i] = 0;
        }
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Update our tracking data each turn
        roundTurn++;
        updateGamePhase(hand);
        suitCounts = calculateSuitCounts(hand);
        updateSevenPlayability(hand, tableLayout);
        
        // Update our hand size in tracking
        playerHandSizes[myID] = hand.size();
        
        // Special case: if this is the first round, consider playing a 7 aggressively
        if (roundTurn <= numPlayers && !hand.empty()) {
            // Look for a strategic 7 to play (prioritize suits with more cards)
            std::vector<std::pair<int, int>> sevenIndices; // (suitCount, cardIndex)
            for (int i = 0; i < static_cast<int>(hand.size()); i++) {
                const Card& card = hand[i];
                if (card.rank == 7 && isCardPlayable(card, tableLayout)) {
                    sevenIndices.push_back({suitCounts[card.suit], i});
                }
            }
            
            if (!sevenIndices.empty()) {
                // Sort by suit count (descending) to prioritize strongest suits
                std::sort(sevenIndices.begin(), sevenIndices.end(),
                          [](const auto& a, const auto& b) { return a.first > b.first; });
                return sevenIndices[0].second;
            }
        }
        
        // Find all playable cards
        std::vector<int> playableCardIndices = findPlayableCards(hand, tableLayout);
        
        // If no playable cards, we have to pass
        if (playableCardIndices.empty()) {
            passesSinceMyLastPlay++;
            return -1;
        }
        
        // If we have exactly one playable card, play it
        if (playableCardIndices.size() == 1) {
            passesSinceMyLastPlay = 0;
            return playableCardIndices[0];
        }
        
        // Calculate scores for each playable card
        std::vector<std::pair<int, double>> cardScores;
        for (int cardIdx : playableCardIndices) {
            double score = evaluateMove(hand[cardIdx], cardIdx, hand, tableLayout);
            cardScores.push_back({cardIdx, score});
        }
        
        // Sort by score (highest first)
        std::sort(cardScores.begin(), cardScores.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Small random chance to not choose the best move to add unpredictability
        if (cardScores.size() >= 2 && std::uniform_real_distribution<>(0, 1)(rng) < 0.05) {
            passesSinceMyLastPlay = 0;
            return cardScores[1].first; // Play the second-best move occasionally
        }
        
        // Return the highest-scoring card index
        passesSinceMyLastPlay = 0;
        return cardScores[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        if (playerID == myID) return; // We already know our own moves
        
        // Add to our list of played cards
        int cardKey = playedCard.suit * 100 + playedCard.rank;
        playedCards.insert(cardKey);
        
        // Update our tracking of player hand sizes
        if (playerID < MAX_PLAYERS && playerHandSizes[playerID] > 0) {
            playerHandSizes[playerID]--;
        }
        
        // Update numPlayers if we discover a new player
        int idAsInt = static_cast<int>(playerID);
        if (idAsInt >= numPlayers) {
            numPlayers = idAsInt + 1;
        }
        
        // Update 7s tracking
        if (playedCard.rank == 7) {
            sevenPlayability[playedCard.suit] = -1; // Mark this 7 as played
        }
    }
    
    void observePass(uint64_t playerID) override {
        if (playerID == myID) return; // We already know when we pass
        
        // Update numPlayers if we discover a new player
        int idAsInt = static_cast<int>(playerID);
        if (idAsInt >= numPlayers) {
            numPlayers = idAsInt + 1;
        }
        
        // Track consecutive passes to detect game state
        if ((playerID + 1) % numPlayers == myID) {
            // The player right before us passed
            consecutivePassesBeforeMe++;
        } else {
            consecutivePassesBeforeMe = 0;
        }
    }
    
    std::string getName() const override {
        return "FYM_Quest";
    }
    
private:
    // Constants for strategy configuration
    static constexpr int MAX_PLAYERS = 8; 
    static constexpr double WEIGHT_SEQUENCE = 1.75;    // Higher weight for sequence play
    static constexpr double WEIGHT_SEVEN = 2.25;       // Higher weight for playing 7s
    static constexpr double WEIGHT_SINGLETON = 1.5;    // Weight for eliminating singletons
    static constexpr double WEIGHT_BLOCKING = 1.8;     // Weight for blocking opponents
    static constexpr double WEIGHT_HAND_REDUCTION = 2.8; // Higher weight for reducing hand size
    static constexpr double WEIGHT_ADAPTIVITY = 1.2;   // Weight for adaptive play based on game state
    static constexpr double WEIGHT_FUTURE_PLAY = 1.6;  // Weight for enabling future plays
    
    // Game state tracking
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn;  // Track which turn of the round we're on
    int gamePhase;  // 0=early, 1=mid, 2=late game
    std::array<int, MAX_PLAYERS> playerHandSizes;  // Track hand sizes
    int numPlayers;  // Number of players detected
    int passesSinceMyLastPlay;  // Track passes since our last play
    int consecutivePassesBeforeMe = 0; // Track consecutive passes before our turn
    
    // Card and suit tracking
    std::array<int, 4> suitCounts;  // Count of cards per suit in hand
    std::array<int, 4> sevenPlayability;  // For each suit, is the 7 playable (1), played (-1), or unplayable (0)
    std::unordered_set<int> playedCards;  // Track which cards have been played (suit*100 + rank)
    
    // Helper methods
    bool isCardPlayable(
        const Card& card,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        int suit = card.suit;
        int rank = card.rank;
        
        // Special case: 7s can be played only if not already on the table
        if (rank == 7) {
            return !(tableLayout.count(suit) > 0 && 
                    tableLayout.at(suit).count(rank) > 0 && 
                    tableLayout.at(suit).at(rank));
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
        
        return higher || lower;
    }
    
    std::vector<int> findPlayableCards(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        std::vector<int> playableCardIndices;
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (isCardPlayable(hand[i], tableLayout)) {
                playableCardIndices.push_back(i);
            }
        }
        return playableCardIndices;
    }
    
    int calculateSequenceLength(
        const Card& card, 
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        int suit = card.suit;
        int rank = card.rank;
        int seqLength = 1; // Start with the card itself
        
        // Check consecutive cards we have in hand in the down direction
        for (int r = rank - 1; r >= 1; r--) {
            // If this rank is already on the table, we can't include it in our sequence
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(r) > 0 && 
                tableLayout.at(suit).at(r)) {
                break;
            }
            
            // Check if we have this card in hand
            bool haveCard = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == r) {
                    haveCard = true;
                    break;
                }
            }
            
            if (!haveCard) break;
            
            // Can only play this card if r+1 is on the table
            if (r < rank - 1) {
                // Need to check if r+1 would be on the table
                bool rPlusOneOnTable = 
                    (tableLayout.count(suit) > 0 && 
                     tableLayout.at(suit).count(r+1) > 0 && 
                     tableLayout.at(suit).at(r+1));
                
                if (!rPlusOneOnTable) break;
            }
            
            seqLength++;
        }
        
        // Check consecutive cards we have in hand in the up direction
        for (int r = rank + 1; r <= 13; r++) {
            // If this rank is already on the table, we can't include it in our sequence
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(r) > 0 && 
                tableLayout.at(suit).at(r)) {
                break;
            }
            
            // Check if we have this card in hand
            bool haveCard = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == r) {
                    haveCard = true;
                    break;
                }
            }
            
            if (!haveCard) break;
            
            // Can only play this card if r-1 is on the table
            if (r > rank + 1) {
                // Need to check if r-1 would be on the table
                bool rMinusOneOnTable = 
                    (tableLayout.count(suit) > 0 && 
                     tableLayout.at(suit).count(r-1) > 0 && 
                     tableLayout.at(suit).at(r-1));
                
                if (!rMinusOneOnTable) break;
            }
            
            seqLength++;
        }
        
        return seqLength;
    }
    
    double evaluateMove(
        const Card& card, 
        int cardIdx, 
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        double score = 0.0;
        
        // 1. Base score for reducing hand size
        score += WEIGHT_HAND_REDUCTION * (1.0 / hand.size());
        
        // 2. Bonus score for playing a 7
        if (card.rank == 7) {
            // Higher bonus for playing 7s with many cards in that suit
            score += WEIGHT_SEVEN * (1.0 + 0.2 * suitCounts[card.suit]);
        }
        
        // 3. Sequence length bonus
        int seqLength = calculateSequenceLength(card, hand, tableLayout);
        score += WEIGHT_SEQUENCE * (seqLength - 1); // -1 because the card itself is counted in seqLength
        
        // 4. Singleton management - encourage playing isolated cards
        if (isSingleton(card, hand)) {
            // Higher bonus in late game for eliminating singletons
            score += WEIGHT_SINGLETON * (1.0 + 0.5 * gamePhase);
        }
        
        // 5. Check if the card will block opponents
        if (willBlockOpponents(card, hand, tableLayout)) {
            score += WEIGHT_BLOCKING;
        }
        
        // 6. Adaptivity: adjust based on game state
        // In early game, prioritize opening sequences
        if (gamePhase == 0) {
            // Prefer playing cards that enable multiple future plays
            int futurePlayCount = countPlayableCardsAfterPlay(card, hand, tableLayout);
            score += WEIGHT_ADAPTIVITY * futurePlayCount * 0.3;
        } 
        // In late game, prioritize getting rid of cards
        else if (gamePhase == 2) {
            // Add bonus for high cards (they're harder to play)
            if (card.rank >= 10 || card.rank <= 2) {
                score += WEIGHT_ADAPTIVITY * 0.5;
            }
        }
        
        // 7. Ensure we don't play a card that leaves us with no future playable cards
        if (!leavesFuturePlay(card, hand, tableLayout) && hand.size() > 1) {
            score -= 5.0; // Significant penalty
        }
        
        // 8. Consider stronger suits (more cards of that suit in hand)
        score += 0.1 * suitCounts[card.suit];
        
        // 9. Add a small random factor to break ties unpredictably
        score += std::uniform_real_distribution<>(0.0, 0.05)(const_cast<std::mt19937&>(rng));
        
        return score;
    }
    
    bool willBlockOpponents(
        const Card& card,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        int suit = card.suit;
        int rank = card.rank;
        
        // Create a simulated table layout with this card added
        auto simulatedLayout = tableLayout;
        simulatedLayout[suit][rank] = true;
        
        // Cards that would be blocked are ones adjacent to our card that aren't in our hand
        // and aren't already on the table
        
        // Check if rank+1 would be blocked
        if (rank < 13) {
            // Already on table?
            bool alreadyOnTable = (tableLayout.count(suit) > 0 && 
                                  tableLayout.at(suit).count(rank+1) > 0 && 
                                  tableLayout.at(suit).at(rank+1));
            
            // In our hand?
            bool inOurHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == rank+1 && cardToIdx(c, hand) != cardToIdx(card, hand)) {
                    inOurHand = true;
                    break;
                }
            }
            
            // If not on table and not in our hand, playing this card would open rank+1
            // which is good for opponents (so not blocking)
            if (!alreadyOnTable && !inOurHand) {
                return false;
            }
        }
        
        // Check if rank-1 would be blocked
        if (rank > 1) {
            // Already on table?
            bool alreadyOnTable = (tableLayout.count(suit) > 0 && 
                                  tableLayout.at(suit).count(rank-1) > 0 && 
                                  tableLayout.at(suit).at(rank-1));
            
            // In our hand?
            bool inOurHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == rank-1 && cardToIdx(c, hand) != cardToIdx(card, hand)) {
                    inOurHand = true;
                    break;
                }
            }
            
            // If not on table and not in our hand, playing this card would open rank-1
            // which is good for opponents (so not blocking)
            if (!alreadyOnTable && !inOurHand) {
                return false;
            }
        }
        
        // If we get here, playing the card doesn't open any new plays for opponents
        return true;
    }
    
    int cardToIdx(const Card& card, const std::vector<Card>& hand) const {
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (hand[i].suit == card.suit && hand[i].rank == card.rank) {
                return i;
            }
        }
        return -1;
    }
    
    bool leavesFuturePlay(
        const Card& card,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        // Create a simulated table layout with this card added
        auto simulatedLayout = tableLayout;
        simulatedLayout[card.suit][card.rank] = true;
        
        // Create a simulated hand without this card
        std::vector<Card> simulatedHand;
        for (const Card& c : hand) {
            if (!(c.suit == card.suit && c.rank == card.rank)) {
                simulatedHand.push_back(c);
            }
        }
        
        // Check if any card in the simulated hand would be playable
        for (const Card& c : simulatedHand) {
            if (isCardPlayable(c, simulatedLayout)) {
                return true;
            }
        }
        
        return false;
    }
    
    int countPlayableCardsAfterPlay(
        const Card& playCard,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const
    {
        // Create a simulated table layout with the played card added
        auto simulatedLayout = tableLayout;
        simulatedLayout[playCard.suit][playCard.rank] = true;
        
        // Create a simulated hand without the played card
        std::vector<Card> simulatedHand;
        for (const Card& c : hand) {
            if (!(c.suit == playCard.suit && c.rank == playCard.rank)) {
                simulatedHand.push_back(c);
            }
        }
        
        // Count playable cards in the simulated state
        int count = 0;
        for (const Card& c : simulatedHand) {
            if (isCardPlayable(c, simulatedLayout)) {
                count++;
            }
        }
        
        return count;
    }
    
    std::array<int, 4> calculateSuitCounts(const std::vector<Card>& hand) const {
        std::array<int, 4> counts = {0, 0, 0, 0};
        for (const Card& card : hand) {
            counts[card.suit]++;
        }
        return counts;
    }
    
    void updateGamePhase(const std::vector<Card>& hand) {
        if (hand.size() > 10) {
            gamePhase = 0; // Early game
        } else if (hand.size() > 5) {
            gamePhase = 1; // Mid game
        } else {
            gamePhase = 2; // Late game
        }
    }
    
    bool isSingleton(const Card& card, const std::vector<Card>& hand) const {
        // Count cards of the same suit
        int suitCount = 0;
        for (const Card& c : hand) {
            if (c.suit == card.suit) {
                suitCount++;
            }
        }
        return suitCount == 1;
    }
    
    int countAdjacentCards(const Card& card, const std::vector<Card>& hand) const {
        int count = 0;
        for (const Card& c : hand) {
            if (c.suit == card.suit && (c.rank == card.rank - 1 || c.rank == card.rank + 1)) {
                count++;
            }
        }
        return count;
    }
    
    void updateSevenPlayability(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout)
    {
        for (int suit = 0; suit < 4; suit++) {
            // Check if the 7 of this suit is already on the table
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(7) > 0 && 
                tableLayout.at(suit).at(7)) {
                sevenPlayability[suit] = -1; // 7 is played
                continue;
            }
            
            // Check if we have the 7 of this suit in hand
            bool haveSevenInHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == 7) {
                    haveSevenInHand = true;
                    break;
                }
            }
            
            if (haveSevenInHand) {
                sevenPlayability[suit] = 1; // 7 is in our hand (playable)
            } else {
                sevenPlayability[suit] = 0; // 7 is neither on table nor in our hand
            }
        }
    }
    
    int getAverageOpponentHandSize() const {
        int sum = 0;
        int count = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (i != static_cast<int>(myID) && playerHandSizes[i] > 0) {
                sum += playerHandSizes[i];
                count++;
            }
        }
        return count > 0 ? sum / count : 13; // Default to 13 if unknown
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
        return new sevens::FYM_Quest();
    }
}
#endif