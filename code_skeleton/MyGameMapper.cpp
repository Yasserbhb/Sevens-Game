#include "MyGameMapper.hpp"
#include "RandomStrategy.hpp" 
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

namespace sevens {

MyGameMapper::MyGameMapper() {
    // Seed the random number generator
    auto seed = static_cast<unsigned long>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    rng.seed(seed);
    
    // Initialize statistics
    total_rounds = 0;
    player_total_cards.clear();
    player_rounds_won.clear();
}

void MyGameMapper::read_cards(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: cannot open file " << filename << std::endl;
        return;
    }

    // Clear existing cards
    cards_hashmap.clear();
    
    // Helper functions to convert text to numeric values
    auto convertRank = [](const std::string& r) -> int {
        if (r == "Jack") return 11;
        if (r == "Queen") return 12;
        if (r == "King") return 13;
        if (r == "Ace") return 1;
        return std::stoi(r);
    };
    
    auto convertSuit = [](const std::string& s) -> int {
        if (s == "Clubs") return 0;
        if (s == "Diamonds") return 1;
        if (s == "Hearts") return 2;
        if (s == "Spades") return 3;
        return -1;
    };
    
    // Read each line from the file
    std::string line;
    uint64_t id = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string rankStr, ofStr, suitStr;
        
        if (!(iss >> rankStr >> ofStr >> suitStr) || ofStr != "of") {
            std::cerr << "Warning: invalid card format: " << line << std::endl;
            continue;
        }
        
        int rank = convertRank(rankStr);
        int suit = convertSuit(suitStr);
        
        if (suit < 0 || rank < 1 || rank > 13) {
            std::cerr << "Warning: invalid card values: " << line << std::endl;
            continue;
        }
        
        cards_hashmap[id] = Card{suit, rank};
        id++;
    }
    
    std::cout << "Parsed " << cards_hashmap.size() << " cards from " << filename << std::endl;
}

void MyGameMapper::read_game(const std::string& /*filename*/) {
    resetTableLayout();
    std::cout << "Game initialized with 7 of Diamonds on the table" << std::endl;
}

void MyGameMapper::resetTableLayout() {
    // Initialize the table layout for all suits
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 1; rank <= 13; rank++) {
            table_layout[suit][rank] = false;
        }
    }
    
    // The game starts with the 7 of Diamonds on the table
    table_layout[1][7] = true; // 1 = Diamonds, 7 = Seven
}

bool MyGameMapper::hasRegisteredStrategies() const {
    return !player_strategies.empty();
}

void MyGameMapper::registerStrategy(uint64_t playerID, std::shared_ptr<PlayerStrategy> strategy) {
    player_strategies[playerID] = strategy;
    strategy->initialize(playerID);
    
    // Initialize statistics for this player
    if (player_total_cards.find(playerID) == player_total_cards.end()) {
        player_total_cards[playerID] = 0;
    }
    if (player_rounds_won.find(playerID) == player_rounds_won.end()) {
        player_rounds_won[playerID] = 0;
    }
}

// Helper function to check if a card is playable
bool MyGameMapper::isPlayable(const Card& card) const {
    int suit = card.suit;
    int rank = card.rank;
    
    // Special case: 7s can be played only if not already on the table
    if (rank == 7) {
        return !(table_layout.count(suit) > 0 && 
                table_layout.at(suit).count(rank) > 0 && 
                table_layout.at(suit).at(rank));
    }
    
    // Check if rank+1 is on the table
    bool higher = (rank < 13 && 
                  table_layout.count(suit) > 0 && 
                  table_layout.at(suit).count(rank+1) > 0 && 
                  table_layout.at(suit).at(rank+1));
                  
    // Check if rank-1 is on the table
    bool lower = (rank > 1 && 
                 table_layout.count(suit) > 0 && 
                 table_layout.at(suit).count(rank-1) > 0 && 
                 table_layout.at(suit).at(rank-1));
    
    return higher || lower;
}

// Main entry point - computes game progress silently
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_game_progress(uint64_t numPlayers) {
    ensureStrategies(numPlayers);
    return runMultipleRounds(false);
}

// Main entry point - computes and displays game
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_and_display_game(uint64_t numPlayers) {
    ensureStrategies(numPlayers);
    return runMultipleRounds(true);
}

// Make sure we have strategies for all players
void MyGameMapper::ensureStrategies(uint64_t numPlayers) {
    for (uint64_t i = 0; i < numPlayers; i++) {
        if (player_strategies.find(i) == player_strategies.end()) {
            auto randomStrat = std::make_shared<RandomStrategy>();
            registerStrategy(i, randomStrat);
        }
    }
}

// Deal cards to players at the start of a round
void MyGameMapper::dealCards() {
    player_hands.clear();
   
    // Convert cards_hashmap to a vector for shuffling, excluding the 7 of Diamonds
    std::vector<std::pair<uint64_t, Card>> cards;
    for (const auto& pair : cards_hashmap) {
        // Skip the 7 of Diamonds (suit=1, rank=7) because it starts on the table
        if (!(pair.second.suit == 1 && pair.second.rank == 7)) {
            cards.push_back(pair);
        }
    }
   
    // Shuffle the cards
    std::shuffle(cards.begin(), cards.end(), rng);
   
    // Deal cards to players
    for (uint64_t i = 0; i < cards.size(); i++) {
        uint64_t playerID = i % player_strategies.size();
        player_hands[playerID].push_back(cards[i].second);
    }
}

// Play multiple rounds until a player accumulates 50 cards
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::runMultipleRounds(bool displayOutput) {
    const uint64_t MAX_ACCUMULATED_CARDS = 5000;
    bool gameOver = false;
    total_rounds = 0;
    
    // Reset statistics
    for (auto& pair : player_total_cards) {
        pair.second = 0;
    }
    for (auto& pair : player_rounds_won) {
        pair.second = 0;
    }
    
    // Keep playing rounds until someone accumulates 50 cards
    while (!gameOver) {
        total_rounds++;
        
        if (displayOutput) {
            std::cout << "\n========== ROUND " << total_rounds << " ==========\n";
        }
        
        // Reset table layout and deal new cards
        resetTableLayout();
        dealCards();
        
        // Play a single round
        uint64_t roundWinner = playRound(displayOutput);
        
        if (roundWinner != UINT64_MAX) {
            // Update round winner stats
            player_rounds_won[roundWinner]++;
            
            if (displayOutput) {
                std::cout << "\nRound " << total_rounds << " Winner: Player " 
                      << roundWinner << " (" << player_strategies[roundWinner]->getName() << ")\n";
            }
        }
        
        // Calculate remaining cards for each player
        if (displayOutput) {
            std::cout << "\nRemaining cards at end of round " << total_rounds << ":\n";
        }
        
        // Check if any player has accumulated 50+ cards
        for (const auto& pair : player_hands) {
            uint64_t playerID = pair.first;
            uint64_t cardsLeft = pair.second.size();
            
            // Add to player's total
            player_total_cards[playerID] += cardsLeft;
            
            if (displayOutput) {
                std::cout << "Player " << playerID << " (" 
                      << player_strategies[playerID]->getName() << "): " 
                      << cardsLeft << " cards (total: " << player_total_cards[playerID] << ")\n";
            }
            
            // Check if this player has reached the limit
            if (player_total_cards[playerID] >= MAX_ACCUMULATED_CARDS) {
                gameOver = true;
            }
        }
        
        // Display table state at end of round
        if (displayOutput) {
            displayTableState();
        }
    }
    
    // Game over - display final results
    if (displayOutput) {
        displayFinalResults();
    }
    
    // Return final standings (sorted by total cards in ascending order)
    return getFinalStandings();
}

// Display final results of the multi-round game
void MyGameMapper::displayFinalResults() {
    std::cout << "\n=================================\n";
    std::cout << "FINAL RESULTS AFTER " << total_rounds << " ROUNDS\n";
    std::cout << "=================================\n";
    
    // Get standings
    auto finalStandings = getFinalStandings();
    
    // Sort by rank
    std::sort(finalStandings.begin(), finalStandings.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Display each player's stats
    for (const auto& pair : finalStandings) {
        uint64_t playerID = pair.first;
        uint64_t rank = pair.second;
        
        double winPercentage = (static_cast<double>(player_rounds_won[playerID]) / total_rounds) * 100.0;
        
        std::cout << "Rank " << rank << ": Player " << playerID << " (" 
              << player_strategies[playerID]->getName() << ")\n";
        std::cout << "    Total Cards: " << player_total_cards[playerID] << "\n";
        std::cout << "    Rounds Won: " << player_rounds_won[playerID] << "/" << total_rounds
              << " (" << std::fixed << std::setprecision(1) << winPercentage << "%)\n";
    }
}

// Get final standings based on total cards (fewer is better)
std::vector<std::pair<uint64_t, uint64_t>> MyGameMapper::getFinalStandings() {
    // Convert map to vector for sorting
    std::vector<std::pair<uint64_t, uint64_t>> cardCounts;
    for (const auto& pair : player_total_cards) {
        cardCounts.push_back(pair);
    }
    
    // Sort by total cards (ascending)
    std::sort(cardCounts.begin(), cardCounts.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Convert to standings (playerID, rank)
    std::vector<std::pair<uint64_t, uint64_t>> standings;
    for (size_t i = 0; i < cardCounts.size(); i++) {
        standings.push_back(std::make_pair(cardCounts[i].first, i + 1));
    }
    
    return standings;
}

// Play a single round until one player empties their hand
uint64_t MyGameMapper::playRound(bool displayOutput) {
    std::vector<uint64_t> player_order;
    
    // Create initial player order
    for (const auto& pair : player_hands) {
        player_order.push_back(pair.first);
    }
    std::sort(player_order.begin(), player_order.end());
    
    uint64_t current_player_idx = 0;
    uint64_t consecutive_passes = 0;
    uint64_t winner = UINT64_MAX; // Invalid player ID to indicate no winner yet
    
    // Game loop - continue until someone empties their hand or the game is blocked
    while (winner == UINT64_MAX) {
        uint64_t playerID = player_order[current_player_idx];
        
        // Skip players who have emptied their hand (should never happen in this implementation)
        if (player_hands[playerID].empty()) {
            current_player_idx = (current_player_idx + 1) % player_order.size();
            continue;
        }
        
        if (displayOutput) {
            std::cout << "Player " << playerID << "'s turn. ";
            std::cout << "Hand size: " << player_hands[playerID].size() << std::endl;
        }
        
        // Get the player's strategy
        auto& strategy = player_strategies[playerID];
        
        // Ask the strategy to select a card
        int card_idx = strategy->selectCardToPlay(player_hands[playerID], table_layout);
        
        if (card_idx == -1 || card_idx >= static_cast<int>(player_hands[playerID].size())) {
            // Player passes
            if (displayOutput) {
                std::cout << "Player " << playerID << " passes" << std::endl;
            }
            
            // Inform other strategies of the pass
            for (const auto& pair : player_strategies) {
                if (pair.first != playerID) {
                    pair.second->observePass(playerID);
                }
            }
            
            consecutive_passes++;
            
            // Check if game is blocked (all players passed)
            if (consecutive_passes >= player_order.size()) {
                // Check if any player has a playable card
                bool gameBlocked = true;
                
                for (const auto& pair : player_hands) {
                    for (const Card& card : pair.second) {
                        if (isPlayable(card)) {
                            gameBlocked = false;
                            break;
                        }
                    }
                    if (!gameBlocked) break;
                }
                
                if (gameBlocked) {
                    // No one can play - round is over with no winner
                    if (displayOutput) {
                        std::cout << "Round is blocked - no valid moves possible" << std::endl;
                    }
                    return UINT64_MAX; // No winner
                }
            }
        } else {
            // Player plays a card
            Card played_card = player_hands[playerID][card_idx];
            
            // Check if the card is actually playable
            if (isPlayable(played_card)) {
                if (displayOutput) {
                    displayCardPlay(playerID, played_card);
                }
                
                // Update the table layout
                table_layout[played_card.suit][played_card.rank] = true;
                
                // Inform other strategies of the move
                for (const auto& pair : player_strategies) {
                    if (pair.first != playerID) {
                        pair.second->observeMove(playerID, played_card);
                    }
                }
                
                // Remove the card from the player's hand
                player_hands[playerID].erase(player_hands[playerID].begin() + card_idx);
                
                // Check if player has emptied their hand - this ends the round
                if (player_hands[playerID].empty()) {
                    if (displayOutput) {
                        std::cout << "Player " << playerID << " has emptied their hand and wins the round!" << std::endl;
                    }
                    winner = playerID;
                    return winner; // Round is over, we have a winner
                }
                
                consecutive_passes = 0;
            } else {
                // Invalid move (card not playable) - treat as a pass
                if (displayOutput) {
                    std::cout << "Player " << playerID << " attempted to play an invalid card. Treated as a pass." << std::endl;
                }
                
                for (const auto& pair : player_strategies) {
                    if (pair.first != playerID) {
                        pair.second->observePass(playerID);
                    }
                }
                
                consecutive_passes++;
            }
        }
        
        // Move to the next player
        current_player_idx = (current_player_idx + 1) % player_order.size();
    }
    
    return winner;
}

// Display a card play
void MyGameMapper::displayCardPlay(uint64_t playerID, const Card& card) {
    std::string suits[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
    std::string ranks[] = {"", "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};
    
    std::cout << "Player " << playerID << " plays " 
          << ranks[card.rank] << " of " << suits[card.suit] << std::endl;
}

// Display the current table state
void MyGameMapper::displayTableState() const {
    std::cout << "\n=== TABLE STATE ===\n";
    
    std::string suit_names[] = {"CLUBS", "DIAMONDS", "HEARTS", "SPADES"};
    
    for (int suit = 0; suit < 4; suit++) {
        std::cout << std::left << std::setw(10) << suit_names[suit] << ": ";
        
        for (int rank = 1; rank <= 13; rank++) {
            if (table_layout.count(suit) > 0 && 
                table_layout.at(suit).count(rank) > 0 && 
                table_layout.at(suit).at(rank)) {
                
                // Special formatting for 7s and face cards
                if (rank == 7) {
                    std::cout << "[7] ";
                } else if (rank == 1) {
                    std::cout << "A ";
                } else if (rank == 11) {
                    std::cout << "J ";
                } else if (rank == 12) {
                    std::cout << "Q ";
                } else if (rank == 13) {
                    std::cout << "K ";
                } else {
                    std::cout << rank << " ";
                }
            }
        }
        std::cout << std::endl;
    }
    std::cout << "=================\n";
}

// Implement the name-based overloads
std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_game_progress(const std::vector<std::string>& playerNames) {
    (void)playerNames;
    throw std::runtime_error("Name-based game progress not implemented");
}

std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_and_display_game(const std::vector<std::string>& playerNames) {
    (void)playerNames;
    throw std::runtime_error("Name-based game display not implemented");
}

} // namespace sevens