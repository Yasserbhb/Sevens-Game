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
 * BalanceStrategy: Cette stratégie maintient un équilibre parfait entre les couleurs
 * dans la main du joueur, priorisant l'élimination des cartes dans les couleurs
 * surreprésentées et conservant celles dans les couleurs sous-représentées.
 * 
 * L'hypothèse: Une distribution équilibrée des couleurs offre une flexibilité maximale
 * tout au long de la partie et évite les blocages tardifs.
 */
class BalanceStrategy : public PlayerStrategy {
public:
    BalanceStrategy() {
        // Initialise le générateur de nombres aléatoires
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    virtual ~BalanceStrategy() = default;
    
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
        
        // Calcule les déséquilibres de couleur dans la main
        auto suitCounts = calculateSuitCounts(hand);
        auto suitImbalances = calculateSuitImbalances(hand, suitCounts);
        
        // Score chaque coup basé sur l'équilibre des couleurs
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = scoreMoveForBalance(idx, hand, tableLayout, suitImbalances);
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Retourne le coup avec le score le plus élevé
        return scoredMoves[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // Cette stratégie ignore les coups des adversaires
        (void)playerID;
        (void)playedCard;
    }
    
    void observePass(uint64_t playerID) override {
        // Cette stratégie ignore les passes des adversaires
        (void)playerID;
    }
    
    std::string getName() const override {
        return "BalanceStrategy";
    }
    
private:
    // Suivi de l'état du jeu
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    
    // Constantes pour l'équilibrage
    static constexpr double BALANCE_WEIGHT = 2.5;      // Poids pour l'équilibre des couleurs
    static constexpr double FUTURE_PLAY_WEIGHT = 1.2;  // Poids pour les coups futurs
    static constexpr double SEVEN_WEIGHT = 1.0;        // Poids pour les 7
    
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
    
    // Compte le nombre de cartes par couleur dans la main
    std::array<int, 4> calculateSuitCounts(const std::vector<Card>& hand) const {
        std::array<int, 4> counts = {0, 0, 0, 0};
        
        for (const Card& card : hand) {
            counts[card.suit]++;
        }
        
        return counts;
    }
    
    // Calcule le déséquilibre de chaque couleur par rapport à la moyenne
    std::array<double, 4> calculateSuitImbalances(
        const std::vector<Card>& hand,
        const std::array<int, 4>& suitCounts) const 
    {
        std::array<double, 4> imbalances = {0.0, 0.0, 0.0, 0.0};
        
        // Si la main est vide ou a une seule carte, retourne des valeurs neutres
        if (hand.size() <= 1) {
            return imbalances;
        }
        
        // Calcule la distribution idéale (équilibrée)
        double idealCount = static_cast<double>(hand.size()) / 4.0;
        
        // Calcule l'écart par rapport à l'idéal pour chaque couleur
        for (int suit = 0; suit < 4; suit++) {
            imbalances[suit] = suitCounts[suit] - idealCount;
        }
        
        return imbalances;
    }
    
    // Détermine la phase du jeu en fonction de la taille de la main
    int getGamePhase(const std::vector<Card>& hand) const {
        if (hand.size() > 10) return 0; // Début de partie
        if (hand.size() > 5) return 1;  // Milieu de partie
        return 2;                       // Fin de partie
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
    
    // Calcule l'impact sur l'équilibre des couleurs si on joue cette carte
    double calculateBalanceImpact(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::array<double, 4>& currentImbalances) const 
    {
        const Card& card = hand[cardIdx];
        int suit = card.suit;
        
        // Si cette couleur est surreprésentée, c'est bon de la jouer
        if (currentImbalances[suit] > 0) {
            return currentImbalances[suit];
        }
        
        // Si cette couleur est sous-représentée, c'est mauvais de la jouer
        if (currentImbalances[suit] < 0) {
            return -currentImbalances[suit];
        }
        
        // Si la couleur est parfaitement équilibrée, impact neutre
        return 0.0;
    }
    
    // Score un coup basé sur l'équilibre des couleurs
    double scoreMoveForBalance(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
        const std::array<double, 4>& suitImbalances) const 
    {
        double score = 1.0; // Score de base
        const Card& card = hand[cardIdx];
        int gamePhase = getGamePhase(hand);
        
        // 1. Impact sur l'équilibre des couleurs - le facteur principal
        double balanceImpact = calculateBalanceImpact(cardIdx, hand, suitImbalances);
        
        // Ajuste l'importance de l'équilibre selon la phase de jeu
        double phaseMultiplier = 1.0;
        if (gamePhase == 0) phaseMultiplier = 0.8;      // Moins important en début de partie
        else if (gamePhase == 2) phaseMultiplier = 1.5; // Plus important en fin de partie
        
        score += BALANCE_WEIGHT * balanceImpact * phaseMultiplier;
        
        // 2. Prise en compte des coups futurs possibles
        int futurePlays = countFuturePlays(cardIdx, hand, tableLayout);
        score += FUTURE_PLAY_WEIGHT * futurePlays * 0.2;
        
        // 3. Traitement spécial pour les 7
        if (card.rank == 7) {
            // Joue les 7 dans les couleurs surreprésentées, garde ceux des couleurs sous-représentées
            if (suitImbalances[card.suit] > 0) {
                score += SEVEN_WEIGHT; // Bonus pour les 7 des couleurs surreprésentées
            } else if (suitImbalances[card.suit] < 0) {
                score -= SEVEN_WEIGHT * 0.5; // Pénalité modérée pour les 7 des couleurs sous-représentées
            }
        }
        
        // 4. Évite de se bloquer
        if (futurePlays == 0 && hand.size() > 1) {
            score -= 3.0; // Forte pénalité
        }
        
        // 5. Petite composante aléatoire pour briser les égalités
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
        return new sevens::BalanceStrategy();
    }
}
#endif