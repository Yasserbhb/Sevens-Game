#pragma once

#include "Generic_game_mapper.hpp"
#include "PlayerStrategy.hpp"
#include <random>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace sevens {

/**
 * Enhanced Sevens card game implementation with:
 * - Multi-round gameplay
 * - Round ends when first player empties their hand
 * - Game ends when a player accumulates 50 cards
 * - Statistics tracking for rounds won and cards accumulated
 */
class MyGameMapper : public Generic_game_mapper {
public:
    MyGameMapper();
    ~MyGameMapper() = default;

    // Required by Generic_game_mapper interface
    std::vector<std::pair<uint64_t, uint64_t>>
    compute_game_progress(uint64_t numPlayers) override;

    std::vector<std::pair<uint64_t, uint64_t>>
    compute_and_display_game(uint64_t numPlayers) override;
    
    std::vector<std::pair<std::string, uint64_t>>
    compute_game_progress(const std::vector<std::string>& playerNames) override;
    
    std::vector<std::pair<std::string, uint64_t>>
    compute_and_display_game(const std::vector<std::string>& playerNames) override;

    // Required by Generic_card_parser and Generic_game_parser
    void read_cards(const std::string& filename) override;
    void read_game(const std::string& filename) override;
    
    // Strategy management
    void registerStrategy(uint64_t playerID, std::shared_ptr<PlayerStrategy> strategy);
    bool hasRegisteredStrategies() const;

private:
    // Game state
    std::mt19937 rng;
    std::unordered_map<uint64_t, std::vector<Card>> player_hands;
    std::unordered_map<uint64_t, std::shared_ptr<PlayerStrategy>> player_strategies;
    
    // Game statistics
    std::unordered_map<uint64_t, uint64_t> player_total_cards;
    std::unordered_map<uint64_t, uint64_t> player_rounds_won;
    uint64_t total_rounds;
    
    // Helper constants
    static const uint64_t INVALID_PLAYER = UINT64_MAX;
    
    // Game setup methods
    void resetTableLayout();
    void ensureStrategies(uint64_t numPlayers);
    void dealCards();
    
    // Game logic methods
    std::vector<std::pair<uint64_t, uint64_t>> runMultipleRounds(bool displayOutput);
    uint64_t playRound(bool displayOutput);
    bool isPlayable(const Card& card) const;
    
    // Display and statistics methods
    void displayCardPlay(uint64_t playerID, const Card& card);
    void displayTableState() const;
    void displayFinalResults();
    std::vector<std::pair<uint64_t, uint64_t>> getFinalStandings();
};

} // namespace sevens