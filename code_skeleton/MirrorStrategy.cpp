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
 * MirrorStrategy: Cette stratégie imite les coups des adversaires et adapte son jeu
 * en fonction des tendances observées à la table.
 * 
 * L'hypothèse: L'imitation des mouvements efficaces des autres joueurs
 * peut être une stratégie supérieure à toute approche fixe.
 */
class MirrorStrategy : public PlayerStrategy {
public:
    MirrorStrategy() {
        // Initialise le générateur de nombres aléatoires
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
        
        // Initialise les suivis de tendances
        for (int suit = 0; suit < 4; suit++) {
            playFrequency[suit] = 0;
        }
    }
    
    virtual ~MirrorStrategy() = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        
        // Réinitialise l'historique des coups observés
        observedMoves.clear();
        
        // Réinitialise les statistiques de jeu
        for (int suit = 0; suit < 4; suit++) {
            playFrequency[suit] = 0;
        }
        
        // Réinitialise les suivi des passes
        playerPasses.clear();
        successPlayers.clear();
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
        
        // Aux premiers tours, avant d'avoir assez d'observations, utilise une stratégie simple
        if (roundTurn <= 3 || observedMoves.size() < 5) {
            return selectEarlyGameMove(playableIndices, hand, tableLayout);
        }
        
        // Score chaque coup basé sur les tendances observées
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = scoreMoveBasedOnTrends(idx, hand, tableLayout);
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Retourne le coup avec le score le plus élevé
        return scoredMoves[0].first;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // Ignore nos propres mouvements
        if (playerID == myID) return;
        
        // Enregistre ce mouvement dans l'historique
        observedMoves.push_front({playerID, playedCard});
        if (observedMoves.size() > MAX_HISTORY_SIZE) {
            observedMoves.pop_back();
        }
        
        // Met à jour les fréquences de jeu par couleur
        playFrequency[playedCard.suit]++;
        
        // Marque ce joueur comme "réussissant à jouer"
        successPlayers.insert(playerID);
        
        // Réinitialise le compteur de passes pour ce joueur
        playerPasses[playerID] = 0;
    }
    
    void observePass(uint64_t playerID) override {
        // Ignore nos propres passes
        if (playerID == myID) return;
        
        // Incrémente le compteur de passes pour ce joueur
        playerPasses[playerID]++;
    }
    
    std::string getName() const override {
        return "MirrorStrategy";
    }
    
private:
    // Constantes
    static constexpr int MAX_HISTORY_SIZE = 30;
    
    // Suivi de l'état du jeu
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    
    // Historique des coups observés (joueur, carte)
    std::deque<std::pair<uint64_t, Card>> observedMoves;
    
    // Fréquence de jeu par couleur
    std::array<int, 4> playFrequency = {0, 0, 0, 0};
    
    // Suivi des passes par joueur
    std::unordered_map<uint64_t, int> playerPasses;
    
    // Ensemble des joueurs qui jouent souvent
    std::set<uint64_t> successPlayers;
    
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
    
    // Stratégie pour les premiers tours avant d'avoir suffisamment d'observations
    int selectEarlyGameMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // Cherche d'abord à jouer un 7 si possible
        for (int idx : playableIndices) {
            if (hand[idx].rank == 7) {
                return idx;
            }
        }
        
        // Sinon, joue une carte au hasard avec une légère préférence pour les cartes centrales
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = 1.0;
            
            // Préfère légèrement les cartes proches de 7
            int distanceFrom7 = std::abs(hand[idx].rank - 7);
            score += 0.1 * (7 - distanceFrom7);
            
            // Facteur aléatoire
            score += std::uniform_real_distribution<>(0.0, 1.0)(const_cast<std::mt19937&>(rng));
            
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return scoredMoves[0].first;
    }
    
    // Analyse quelles couleurs sont jouées le plus souvent
    std::array<double, 4> analyzeSuitTrends() const {
        std::array<double, 4> trends = {0.0, 0.0, 0.0, 0.0};
        
        // Si aucune donnée, retourne des tendances neutres
        if (observedMoves.empty()) {
            return trends;
        }
        
        // Calcule la somme totale pour normalisation
        int totalPlays = 0;
        for (int f : playFrequency) {
            totalPlays += f;
        }
        
        if (totalPlays > 0) {
            // Normalise les fréquences de jeu
            for (int suit = 0; suit < 4; suit++) {
                trends[suit] = static_cast<double>(playFrequency[suit]) / totalPlays;
            }
        }
        
        return trends;
    }
    
    // Analyse quels rangs sont joués le plus souvent
    std::array<double, 13> analyzeRankTrends() const {
        std::array<double, 13> trends = {0.0};
        
        // Compte la fréquence de chaque rang
        std::array<int, 13> rankCounts = {0};
        for (const auto& move : observedMoves) {
            rankCounts[move.second.rank - 1]++;
        }
        
        // Calcule la somme totale pour normalisation
        int totalPlays = 0;
        for (int count : rankCounts) {
            totalPlays += count;
        }
        
        if (totalPlays > 0) {
            // Normalise les fréquences
            for (int rank = 0; rank < 13; rank++) {
                trends[rank] = static_cast<double>(rankCounts[rank]) / totalPlays;
            }
        }
        
        return trends;
    }
    
    // Analyse quels joueurs réussissent le mieux (passent le moins)
    std::unordered_map<uint64_t, double> analyzePlayerSuccess() const {
        std::unordered_map<uint64_t, double> successRates;
        
        for (const auto& entry : playerPasses) {
            uint64_t playerID = entry.first;
            int passes = entry.second;
            
            // Calcule un taux de succès basé sur les passes
            // Plus un joueur passe, moins il est considéré comme réussissant
            double successRate = 1.0 / (1.0 + passes);
            successRates[playerID] = successRate;
        }
        
        return successRates;
    }
    
    // Évalue si un coup est similaire aux coups récemment joués par les joueurs qui réussissent
    double evaluateSimilarityToSuccessfulPlayers(
        int cardIdx,
        const std::vector<Card>& hand) const 
    {
        const Card& card = hand[cardIdx];
        double similarity = 0.0;
        
        // Si nous n'avons pas encore assez d'observations, retourne une valeur neutre
        if (observedMoves.size() < 3 || successPlayers.empty()) {
            return 0.5;
        }
        
        // Analyse les coups des joueurs qui réussissent
        for (const auto& move : observedMoves) {
            uint64_t playerID = move.first;
            const Card& playedCard = move.second;
            
            // Vérifie si ce joueur est considéré comme réussissant
            if (successPlayers.count(playerID) > 0) {
                // Calcule la similarité entre notre carte et celle jouée
                double moveSimil = 0.0;
                
                // Même couleur
                if (playedCard.suit == card.suit) {
                    moveSimil += 0.5;
                }
                
                // Rang proche
                int rankDiff = std::abs(playedCard.rank - card.rank);
                if (rankDiff <= 2) {
                    moveSimil += 0.3 * (1.0 - rankDiff / 13.0);
                }
                
                // Même type (7, extrême, milieu)
                bool bothSevens = (playedCard.rank == 7 && card.rank == 7);
                bool bothExtremes = ((playedCard.rank <= 3 || playedCard.rank >= 11) &&
                                    (card.rank <= 3 || card.rank >= 11));
                bool bothMiddle = ((playedCard.rank > 3 && playedCard.rank < 11) &&
                                  (card.rank > 3 && card.rank < 11));
                
                if (bothSevens || bothExtremes || bothMiddle) {
                    moveSimil += 0.2;
                }
                
                // Ajoute cette similarité au score total
                similarity += moveSimil;
            }
        }
        
        // Normalise par le nombre de coups observés
        if (!observedMoves.empty()) {
            similarity /= observedMoves.size();
        }
        
        return similarity;
    }
    
    // Score un coup basé sur les tendances observées
    double scoreMoveBasedOnTrends(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        double score = 1.0; // Score de base
        const Card& card = hand[cardIdx];
        
        // 1. Analyse les tendances de couleur
        auto suitTrends = analyzeSuitTrends();
        
        // Préfère jouer dans les couleurs populaires
        score += 1.5 * suitTrends[card.suit];
        
        // 2. Analyse les tendances de rang
        auto rankTrends = analyzeRankTrends();
        
        // Préfère jouer des rangs populaires
        score += 1.0 * rankTrends[card.rank - 1];
        
        // 3. Évalue la similarité avec les joueurs qui réussissent
        double similarity = evaluateSimilarityToSuccessfulPlayers(cardIdx, hand);
        score += 2.0 * similarity;
        
        // 4. Considération pratique: nombre de coups futurs possibles
        int futurePlays = countFuturePlays(cardIdx, hand, tableLayout);
        score += 0.5 * futurePlays;
        
        // 5. Évite de se bloquer
        if (futurePlays == 0 && hand.size() > 1) {
            score -= 3.0; // Forte pénalité
        }
        
        // 6. Facteur aléatoire pour éviter d'être trop prévisible
        // Le "délai d'imitation" introduit une variabilité contrôlée
        score += std::uniform_real_distribution<>(0.0, 0.3)(const_cast<std::mt19937&>(rng));
        
        return score;
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
};

} // namespace sevens

#ifdef BUILDING_DLL
extern "C" {
    // Export function for dynamic library loading - works on both Windows and Linux
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    sevens::PlayerStrategy* createStrategy() {
        return new sevens::MirrorStrategy();
    }
}
#endif