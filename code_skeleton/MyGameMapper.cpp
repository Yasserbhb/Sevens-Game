#include "MyGameMapper.hpp"
#include "RandomStrategy.hpp" 
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

namespace sevens {

MyGameMapper::MyGameMapper() {
    // Seed the random engine for shuffling cards
    auto seed = static_cast<unsigned long>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    rng.seed(seed);
}

void MyGameMapper::read_cards(const std::string& filename) {
    // Read cards directly
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
    // Initialize the table layout for all suits
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 1; rank <= 13; rank++) {
            table_layout[suit][rank] = false;
        }
    }
    
    // The game starts with the 7 of Diamonds on the table
    table_layout[1][7] = true; // 1 = Diamonds, 7 = Seven
    
    std::cout << "Game initialized with 7 of Diamonds on the table" << std::endl;
}

bool MyGameMapper::hasRegisteredStrategies() const {
    return !player_strategies.empty();
}

void MyGameMapper::registerStrategy(uint64_t playerID, std::shared_ptr<PlayerStrategy> strategy) {
    player_strategies[playerID] = strategy;
    strategy->initialize(playerID);
}

// Helper function to check if a card is playable

bool MyGameMapper::isPlayable(const Card& card) const {
    int suit = card.suit;
    int rank = card.rank;
    
    // Special case: 7s can be played only if not already on the table
    if (rank == 7) {
        // Check if this 7 is already on the table
        return !(table_layout.count(suit) > 0 && 
                table_layout.at(suit).count(rank) > 0 && 
                table_layout.at(suit).at(rank));
    }
    
    // Normal case: adjacent to existing cards
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

std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_game_progress(uint64_t numPlayers) {
    // Initialize player hands and strategies
    initializeGame(numPlayers);
    
    // Play the game silently (no output)
    return playGame(false);
}

std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_and_display_game(uint64_t numPlayers) {
    // Initialize player hands and strategies
    initializeGame(numPlayers);
    
    // Play the game with output
    return playGame(true);
}

// Helper to initialize the game
void MyGameMapper::initializeGame(uint64_t numPlayers) {
    // Clear any previous game state
    player_hands.clear();
    if (player_strategies.size() < numPlayers) {
        // Create default strategies for any players who don't have one registered
        for (uint64_t i = 0; i < numPlayers; i++) {
            if (player_strategies.find(i) == player_strategies.end()) {
                auto randomStrat = std::make_shared<RandomStrategy>();
                registerStrategy(i, randomStrat);
            }
        }
    }
    
    // Convert our cards_hashmap to a vector for shuffling
    std::vector<std::pair<uint64_t, Card>> cards;
    for (const auto& pair : cards_hashmap) {
        cards.push_back(pair);
    }
    
    // Shuffle the cards
    std::shuffle(cards.begin(), cards.end(), rng);
    
    // Deal cards to players
    for (uint64_t i = 0; i < cards.size(); i++) {
        uint64_t playerID = i % numPlayers;
        player_hands[playerID].push_back(cards[i].second);
    }
}

// Main game loop
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::playGame(bool displayOutput) {
    std::vector<uint64_t> player_order;
    std::vector<std::pair<uint64_t, uint64_t>> results;
    std::vector<uint64_t> finished_players;
    
    // Create initial player order
    for (const auto& pair : player_hands) {
        player_order.push_back(pair.first);
    }
    std::sort(player_order.begin(), player_order.end());
    
    uint64_t current_player_idx = 0;
    uint64_t consecutive_passes = 0;
    uint64_t num_players = player_order.size();
    
    // Game loop
    while (finished_players.size() < num_players) {
        uint64_t playerID = player_order[current_player_idx];
        
        // Skip players who have finished
        if (std::find(finished_players.begin(), finished_players.end(), playerID) != finished_players.end()) {
            current_player_idx = (current_player_idx + 1) % num_players;
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
            
            // In playGame method, modify the consecutive passes check:
            consecutive_passes++;
            if (consecutive_passes >= player_order.size() - finished_players.size()) {
                // Check if the game is truly blocked by checking if any player has a playable card
                bool gameBlocked = true;
                
                for (const auto& pair : player_hands) {
                    uint64_t playerID = pair.first;
                    
                    // Skip finished players
                    if (std::find(finished_players.begin(), finished_players.end(), playerID) != finished_players.end()) {
                        continue;
                    }
                    
                    const auto& playerHand = pair.second;
                    for (const Card& card : playerHand) {
                        if (isPlayable(card)) {
                            // If any player has a playable card, the game is not blocked
                            gameBlocked = false;
                            break;
                        }
                    }
                    
                    if (!gameBlocked) {
                        break;
                    }
                }
                
                if (gameBlocked) {
                    // The game is truly blocked - no player can make a valid move
                    if (displayOutput) {
                        std::cout << "Game blocked: all players passed and no valid moves possible" << std::endl;
                    }
                    break;
                } else {
                    // Reset consecutive passes since someone might be able to play on their next turn
                    consecutive_passes = 0;
                }
            }
        } else {
            // Player plays a card
            Card& played_card = player_hands[playerID][card_idx];
            
            // Check if the card is actually playable
            if (isPlayable(played_card)) {
                if (displayOutput) {
                    std::string ranks[] = {"", "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};
                    std::string suits[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
                    std::cout << "Player " << playerID << " plays " << ranks[played_card.rank] << " of " << suits[played_card.suit] << std::endl;
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
                
                // Check if player has emptied their hand
                if (player_hands[playerID].empty()) {
                    if (displayOutput) {
                        std::cout << "Player " << playerID << " has emptied their hand and finishes in position " 
                                << (finished_players.size() + 1) << std::endl;
                    }
                    finished_players.push_back(playerID);
                    results.push_back(std::make_pair(playerID, finished_players.size()));
                }
                
                consecutive_passes = 0;
            } else {
                // Invalid move (card not playable) - treat as a pass
                if (displayOutput) {
                    std::string ranks[] = {"", "Ace", "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King"};
                    std::string suits[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
                    std::cout << "Player " << playerID << " attempted to play an invalid card: " 
                            << ranks[played_card.rank] << " of " << suits[played_card.suit] << ". Treated as a pass." << std::endl;
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
        current_player_idx = (current_player_idx + 1) % num_players;
        // In the playGame method, after a card is played and the table is updated:

    }
    
    // Add any remaining players to the results
    for (uint64_t playerID : player_order) {
        if (std::find_if(results.begin(), results.end(), 
                        [playerID](const std::pair<uint64_t, uint64_t>& p) { return p.first == playerID; }) == results.end()) {
            results.push_back(std::make_pair(playerID, 0)); // 0 indicates did not finish
        }
    }
    
    return results;
}

// Implement the name-based overloads (throwing exceptions is fine for now)
std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_game_progress(const std::vector<std::string>& playerNames) {
    // Silence the warning
    (void)playerNames;
    
    throw std::runtime_error("Name-based game progress not implemented");
}

std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_and_display_game(const std::vector<std::string>& playerNames) {
    // Silence the warning
    (void)playerNames;
    
    throw std::runtime_error("Name-based game display not implemented");
}

} // namespace sevens