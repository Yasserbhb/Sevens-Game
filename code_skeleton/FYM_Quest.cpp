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
 * UltimateStrategy: Une stratégie optimisée basée sur l'analyse de 55345 manches
 * 
 * Principes fondamentaux:
 * 1. Prioritise la construction de séquences (comme SequenceStrategy)
 * 2. Intègre un élément contrôlé d'aléatoire (inspiré par la robustesse de RandomStrategy)
 * 3. Maintient la simplicité (évite la suroptimisation qui a nui aux stratégies complexes)
 * 4. Se concentre sur le jeu personnel plutôt que sur le blocage
 */
class UltimateStrategy : public PlayerStrategy {
public:
    UltimateStrategy() {
        // Initialise le générateur de nombres aléatoires avec l'horloge système
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    virtual ~UltimateStrategy() = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Incrémente le compteur de tours
        roundTurn++;
        
        // Trouve toutes les cartes jouables
        std::vector<int> playableIndices = findPlayableCards(hand, tableLayout);
        
        // Si aucune carte jouable, on doit passer
        if (playableIndices.empty()) {
            return -1;
        }
        
        // Si une seule carte jouable, la jouer
        if (playableIndices.size() == 1) {
            return playableIndices[0];
        }
        
        // Calcule les forces des couleurs
        auto suitStrengths = calculateSuitStrengths(hand);
        
        // Premier tour - stratégie spéciale pour ouvrir la partie
        if (roundTurn <= 2) {
            return selectFirstTurnMove(playableIndices, hand, tableLayout, suitStrengths);
        }
        
        // Calcule les scores pour chaque carte jouable
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = scoreMove(idx, hand, tableLayout, suitStrengths);
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Élément d'aléatoire contrôlé, basé sur la robustesse démontrée par RandomStrategy
        if (scoredMoves.size() >= 2 && std::uniform_real_distribution<>(0.0, 1.0)(rng) < 0.15) {
            // 15% de chances de choisir le deuxième meilleur coup
            return scoredMoves[1].first;
        }
        
        // Retourne le coup avec le score le plus élevé
        return scoredMoves[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // Observe simplement pour des analyses futures
        (void)playerID;
        (void)playedCard;
    }
    
    void observePass(uint64_t playerID) override {
        // Observe simplement pour des analyses futures
        (void)playerID;
    }
    
    std::string getName() const override {
        return "UltimateStrategy";
    }
    
private:
    // Suivi de l'état du jeu
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    
    // Constantes de stratégie
    static constexpr double SEQUENCE_WEIGHT = 2.5;    // Poids élevé pour les séquences
    static constexpr double SEVEN_WEIGHT = 1.2;       // Poids modéré pour les 7
    static constexpr double EXTREMES_WEIGHT = 1.0;    // Poids pour les cartes extrêmes (A,2,3,J,Q,K)
    static constexpr double FUTURE_PLAYS_WEIGHT = 1.5; // Poids pour les coups futurs
    
    // Trouve toutes les cartes jouables dans la main
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
    
    // Vérifie si une carte est jouable sur la table actuelle
    bool isCardPlayable(
        const Card& card,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        int suit = card.suit;
        int rank = card.rank;
        
        // Les 7 sont jouables s'ils ne sont pas déjà sur la table
        if (rank == 7) {
            return !(tableLayout.count(suit) > 0 && 
                    tableLayout.at(suit).count(rank) > 0 && 
                    tableLayout.at(suit).at(rank));
        }
        
        // Vérifie si rank+1 est sur la table (peut jouer une carte inférieure)
        bool higherOnTable = (rank < 13 && 
                            tableLayout.count(suit) > 0 && 
                            tableLayout.at(suit).count(rank+1) > 0 && 
                            tableLayout.at(suit).at(rank+1));
                            
        // Vérifie si rank-1 est sur la table (peut jouer une carte supérieure)
        bool lowerOnTable = (rank > 1 && 
                            tableLayout.count(suit) > 0 && 
                            tableLayout.at(suit).count(rank-1) > 0 && 
                            tableLayout.at(suit).at(rank-1));
        
        return higherOnTable || lowerOnTable;
    }
    
    // Calcule la force de chaque couleur dans la main
    std::array<int, 4> calculateSuitStrengths(const std::vector<Card>& hand) const {
        std::array<int, 4> strengths = {0, 0, 0, 0};
        
        for (const Card& card : hand) {
            strengths[card.suit]++;
        }
        
        return strengths;
    }
    
    // Sélectionne le meilleur coup pour les premiers tours
    int selectFirstTurnMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        const std::array<int, 4>& suitStrengths) const 
    {
        // Vérifie d'abord si on peut jouer un 7 dans une couleur forte
        for (int idx : playableIndices) {
            const Card& card = hand[idx];
            if (card.rank == 7 && suitStrengths[card.suit] >= 3) {
                return idx; // Joue immédiatement un 7 dans une couleur forte
            }
        }
        
        // Vérifie ensuite toute séquence de longueur 3+
        for (int idx : playableIndices) {
            int seqLength = calculateSequenceLength(idx, hand, tableLayout);
            if (seqLength >= 3) {
                return idx; // Priorité aux longues séquences
            }
        }
        
        // Sinon, évalue normalement avec un accent sur les séquences
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = 1.0; // Score de base
            const Card& card = hand[idx];
            
            // Bonus pour les 7 (mais pas aussi élevé que dans d'autres stratégies)
            if (card.rank == 7) {
                score += SEVEN_WEIGHT;
                
                // Ajuste en fonction de la force de la couleur
                score += 0.2 * suitStrengths[card.suit];
            }
            
            // Fort bonus pour les séquences
            int seqLength = calculateSequenceLength(idx, hand, tableLayout);
            score += SEQUENCE_WEIGHT * (seqLength - 1) * 0.5;
            
            // Bonus pour les coups futurs potentiels
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            score += 0.4 * futurePlays;
            
            // Bonus pour les cartes proches du 7 (préférence pour les cartes centrales)
            int distanceFrom7 = std::abs(card.rank - 7);
            score += 0.1 * (7 - distanceFrom7);
            
            // Facteur aléatoire pour briser les égalités
            score += std::uniform_real_distribution<>(0.0, 0.2)(const_cast<std::mt19937&>(rng));
            
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return scoredMoves[0].first;
    }
    
    // Calcule la longueur de séquence potentielle pour une carte
    int calculateSequenceLength(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        int suit = card.suit;
        int rank = card.rank;
        int length = 1; // Commence avec la carte elle-même
        
        // Crée une table simulée avec cette carte jouée
        auto simulatedTable = tableLayout;
        simulatedTable[suit][rank] = true;
        
        // Recherche des cartes que nous avons en séquence en-dessous de cette carte
        for (int r = rank - 1; r >= 1; r--) {
            bool haveCardInHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == r) {
                    haveCardInHand = true;
                    break;
                }
            }
            
            if (!haveCardInHand) break;
            
            // Vérifie si cette carte serait jouable après avoir joué notre carte
            if (r == rank - 1) {
                // Directement adjacente à notre carte - serait jouable
                length++;
            } else {
                // Vérifie si r+1 serait sur la table
                if (simulatedTable.count(suit) > 0 && 
                    simulatedTable.at(suit).count(r+1) > 0 && 
                    simulatedTable.at(suit).at(r+1)) {
                    length++;
                } else {
                    break;
                }
            }
        }
        
        // Réinitialise la table simulée
        simulatedTable = tableLayout;
        simulatedTable[suit][rank] = true;
        
        // Recherche des cartes que nous avons en séquence au-dessus de cette carte
        for (int r = rank + 1; r <= 13; r++) {
            bool haveCardInHand = false;
            for (const Card& c : hand) {
                if (c.suit == suit && c.rank == r) {
                    haveCardInHand = true;
                    break;
                }
            }
            
            if (!haveCardInHand) break;
            
            // Vérifie si cette carte serait jouable après avoir joué notre carte
            if (r == rank + 1) {
                // Directement adjacente à notre carte - serait jouable
                length++;
            } else {
                // Vérifie si r-1 serait sur la table
                if (simulatedTable.count(suit) > 0 && 
                    simulatedTable.at(suit).count(r-1) > 0 && 
                    simulatedTable.at(suit).at(r-1)) {
                    length++;
                } else {
                    break;
                }
            }
        }
        
        return length;
    }
    
    // Compte les coups futurs après avoir joué une carte
    int countFuturePlays(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // Crée une table simulée avec cette carte jouée
        auto simulatedTable = tableLayout;
        const Card& playedCard = hand[cardIdx];
        simulatedTable[playedCard.suit][playedCard.rank] = true;
        
        // Compte combien de nos cartes restantes seraient jouables
        int count = 0;
        for (int i = 0; i < static_cast<int>(hand.size()); i++) {
            if (i != cardIdx && isCardPlayable(hand[i], simulatedTable)) {
                count++;
            }
        }
        
        return count;
    }
    
    // Vérifie si une carte est extrême (As, 2, 3 ou Valet, Dame, Roi)
    bool isExtremeCard(const Card& card) const {
        return card.rank <= 3 || card.rank >= 11;
    }
    
    // Vérifie si une carte est un singleton (seule carte de sa couleur dans la main)
    bool isSingleton(const Card& card, const std::vector<Card>& hand) const {
        int suitCount = 0;
        for (const Card& c : hand) {
            if (c.suit == card.suit) {
                suitCount++;
            }
        }
        return suitCount == 1;
    }
    
    // Évalue un coup potentiel en fonction de multiples facteurs stratégiques
    double scoreMove(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        const std::array<int, 4>& suitStrengths) const 
    {
        double score = 1.0; // Score de base
        const Card& card = hand[cardIdx];
        
        // 1. Séquences - facteur le plus important basé sur les résultats
        int seqLength = calculateSequenceLength(cardIdx, hand, tableLayout);
        score += SEQUENCE_WEIGHT * (seqLength - 1) * 0.5;
        
        // 2. Gestion des 7 - importante mais pas dominante
        if (card.rank == 7) {
            double sevenScore = SEVEN_WEIGHT;
            
            // Ajuste en fonction de la force de la couleur
            if (suitStrengths[card.suit] >= 3) {
                sevenScore += 0.5; // Bonus pour les 7 dans une couleur forte
            } else if (suitStrengths[card.suit] == 1) {
                sevenScore -= 0.5; // Légère pénalité pour les 7 solitaires
            }
            
            score += sevenScore;
        }
        
        // 3. Opportunités de jeu futures - important pour maximiser les options
        int futurePlays = countFuturePlays(cardIdx, hand, tableLayout);
        score += FUTURE_PLAYS_WEIGHT * futurePlays * 0.3;
        
        // 4. Cartes extrêmes - plus difficiles à jouer
        if (isExtremeCard(card)) {
            score += EXTREMES_WEIGHT;
        }
        
        // 5. Gestion des singletons
        if (isSingleton(card, hand)) {
            score += 1.0; // Bonus pour éliminer les singletons
        }
        
        // 6. Préférence pour les cartes de la couleur la plus forte
        score += 0.1 * suitStrengths[card.suit];
        
        // 7. Éviter de nous laisser sans coups futurs
        if (futurePlays == 0 && hand.size() > 1) {
            score -= 2.0; // Forte pénalité
        }
        
        // 8. Facteur aléatoire pour briser les égalités et ajouter de l'imprévisibilité
        score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
        
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
        return new sevens::UltimateStrategy();
    }
}
#endif