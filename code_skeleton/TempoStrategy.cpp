#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <chrono>
#include <string>
#include <map>

namespace sevens {

/**
 * TempoStrategy: Cette stratégie manipule activement le rythme du jeu,
 * alternant entre accélération pour ouvrir des opportunités et ralentissement
 * pour contrôler le flux de la partie.
 * 
 * L'hypothèse: Contrôler le tempo du jeu permet de dominer le flux de la partie
 * et de neutraliser les stratégies adverses.
 */
class TempoStrategy : public PlayerStrategy {
public:
    TempoStrategy() {
        // Initialise le générateur de nombres aléatoires
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    virtual ~TempoStrategy() = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
        gamePhase = GamePhase::Opening;
        
        // Réinitialise le suivi des joueurs
        for (int i = 0; i < MAX_PLAYERS; i++) {
            opponentHandSizes[i] = -1;
        }
        
        // Réinitialise l'état des 7
        for (int suit = 0; suit < 4; suit++) {
            sevenStatus[suit] = 0; // 0 = inconnu/non joué
        }
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Incrémente le compteur de tours
        roundTurn++;
        
        // Met à jour le statut des 7
        updateSevenStatus(hand, tableLayout);
        
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
        
        // Détermine la phase de jeu en fonction de l'état actuel
        updateGamePhase(hand, tableLayout);
        
        // Sélectionne la carte à jouer selon la phase actuelle
        switch (gamePhase) {
            case GamePhase::Opening:
                return selectOpeningMove(playableIndices, hand, tableLayout);
            case GamePhase::ControlPhase:
                return selectControlMove(playableIndices, hand, tableLayout);
            case GamePhase::Endgame:
                return selectEndgameMove(playableIndices, hand, tableLayout);
            default:
                // Ne devrait jamais arriver, mais au cas où
                return playableIndices[0];
        }
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // Ignore nos propres mouvements
        if (playerID == myID) return;
        
        // Met à jour le statut des 7 joués
        if (playedCard.rank == 7) {
            sevenStatus[playedCard.suit] = -1; // -1 = joué
        }
        
        // Met à jour la taille de main estimée pour ce joueur
        if (playerID < MAX_PLAYERS) {
            if (opponentHandSizes[playerID] == -1) {
                // Première observation de ce joueur, initialise à une valeur par défaut
                opponentHandSizes[playerID] = 13; // Taille de main typique au début
            }
            
            // Le joueur a joué une carte, sa main diminue
            opponentHandSizes[playerID] = std::max(0, opponentHandSizes[playerID] - 1);
        }
        
        // Enregistre le dernier joueur actif
        lastActivePlayer = playerID;
        
        // Réinitialise le compteur de passes consécutives
        consecutivePasses = 0;
    }
    
    void observePass(uint64_t playerID) override {
        // Ignore nos propres passes
        if (playerID == myID) return;
        
        // Incrémente le compteur de passes consécutives
        consecutivePasses++;
        
        // Enregistre le dernier joueur qui a passé
        lastPassingPlayer = playerID;
    }
    
    std::string getName() const override {
        return "TempoStrategy";
    }
    
private:
    // Constantes
    static constexpr int MAX_PLAYERS = 10;
    
    // Phase de jeu (contrôle du tempo)
    enum class GamePhase {
        Opening,      // Phase d'ouverture - accélère le jeu
        ControlPhase, // Phase de contrôle - ralentit le jeu
        Endgame       // Phase finale - maximise l'efficacité
    };
    
    // Suivi de l'état du jeu
    uint64_t myID;
    std::mt19937 rng;
    int roundTurn = 0;
    GamePhase gamePhase = GamePhase::Opening;
    int consecutivePasses = 0;
    uint64_t lastActivePlayer = 0;
    uint64_t lastPassingPlayer = 0;
    
    // Suivi des adversaires
    std::array<int, MAX_PLAYERS> opponentHandSizes = {-1}; // -1 = inconnu
    
    // Statut des 7 (0 = inconnu/non joué, 1 = dans notre main, -1 = joué)
    std::array<int, 4> sevenStatus = {0, 0, 0, 0};
    
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
    
    // Met à jour le statut des 7 dans notre main et sur la table
    void updateSevenStatus(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        // Vérifie chaque couleur
        for (int suit = 0; suit < 4; suit++) {
            // Est-ce que le 7 de cette couleur est sur la table?
            if (tableLayout.count(suit) > 0 && 
                tableLayout.at(suit).count(7) > 0 && 
                tableLayout.at(suit).at(7)) {
                sevenStatus[suit] = -1; // 7 est sur la table
                continue;
            }
            
            // Avons-nous le 7 de cette couleur dans notre main?
            bool have7 = false;
            for (const Card& card : hand) {
                if (card.suit == suit && card.rank == 7) {
                    have7 = true;
                    break;
                }
            }
            
            if (have7) {
                sevenStatus[suit] = 1; // 7 est dans notre main
            } else if (sevenStatus[suit] != -1) {
                sevenStatus[suit] = 0; // 7 n'est ni sur la table ni dans notre main
            }
        }
    }
    
    // Compte les couleurs ouvertes sur la table
    int countOpenedSuits(const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const {
        int count = 0;
        for (int suit = 0; suit < 4; suit++) {
            if (tableLayout.count(suit) > 0 && !tableLayout.at(suit).empty()) {
                count++;
            }
        }
        return count;
    }
    
    // Met à jour la phase de jeu en fonction de la situation actuelle
    void updateGamePhase(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        // Nombre de couleurs ouvertes
        int openSuits = countOpenedSuits(tableLayout);
        
        // Détermine la phase en fonction de plusieurs facteurs
        if (hand.size() <= 5 || roundTurn > 20) {
            // Fin de partie - concentré sur l'efficacité
            gamePhase = GamePhase::Endgame;
        } else if (openSuits <= 1 || roundTurn < 5) {
            // Début de partie ou peu de couleurs ouvertes - phase d'ouverture
            gamePhase = GamePhase::Opening;
        } else {
            // Phase intermédiaire - contrôle le tempo
            
            // Si beaucoup de passes consécutives, revient en phase d'ouverture
            if (consecutivePasses >= 3) {
                gamePhase = GamePhase::Opening;
            } else {
                // Par défaut en phase de contrôle
                gamePhase = GamePhase::ControlPhase;
                
                // Vérifie s'il y a un adversaire avec très peu de cartes
                for (int size : opponentHandSizes) {
                    if (size > 0 && size <= 3 && size < hand.size() - 1) {
                        // Un adversaire a peu de cartes - ralentir davantage
                        gamePhase = GamePhase::ControlPhase;
                        break;
                    }
                }
            }
        }
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
    
    // Vérifie si un coup est susceptible d'aider les adversaires
    bool willEnableOpponentPlays(
        int cardIdx,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        const Card& card = hand[cardIdx];
        
        // Les 7 créent toujours de nouvelles opportunités (dans les deux directions)
        if (card.rank == 7) {
            return true;
        }
        
        // Pour les autres cartes, vérifier si elles créent de nouveaux endpoints
        if (card.rank > 1 && card.rank < 13) {
            // Vérifie si rank-1 est déjà sur la table
            bool lowerAlreadyOnTable = (tableLayout.count(card.suit) > 0 && 
                                       tableLayout.at(card.suit).count(card.rank-1) > 0 && 
                                       tableLayout.at(card.suit).at(card.rank-1));
                                       
            // Vérifie si rank+1 est déjà sur la table
            bool higherAlreadyOnTable = (tableLayout.count(card.suit) > 0 && 
                                        tableLayout.at(card.suit).count(card.rank+1) > 0 && 
                                        tableLayout.at(card.suit).at(card.rank+1));
            
            // Si cette carte crée un nouvel endpoint, elle pourrait aider les adversaires
            return (!lowerAlreadyOnTable && card.rank > 1) || 
                   (!higherAlreadyOnTable && card.rank < 13);
        }
        
        return false;
    }
    
    // Sélectionne le meilleur coup pour la phase d'ouverture
    int selectOpeningMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // Dans la phase d'ouverture, priorité absolue aux 7 pour ouvrir de nouvelles couleurs
        std::vector<int> sevenIndices;
        for (int idx : playableIndices) {
            if (hand[idx].rank == 7) {
                sevenIndices.push_back(idx);
            }
        }
        
        // S'il y a des 7 jouables, prendre celui qui nous laisse le plus d'options futures
        if (!sevenIndices.empty()) {
            if (sevenIndices.size() == 1) {
                return sevenIndices[0];
            }
            
            // Compare les options futures après avoir joué chaque 7
            int bestIdx = sevenIndices[0];
            int maxFuturePlays = countFuturePlays(bestIdx, hand, tableLayout);
            
            for (int idx : sevenIndices) {
                int futurePlays = countFuturePlays(idx, hand, tableLayout);
                if (futurePlays > maxFuturePlays) {
                    maxFuturePlays = futurePlays;
                    bestIdx = idx;
                }
            }
            
            return bestIdx;
        }
        
        // Pas de 7 - Score chaque coup pour maximiser les options et l'ouverture
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = 1.0; // Score de base
            
            // 1. Préfère les cartes qui ouvrent de nouvelles possibilités
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            score += 1.5 * futurePlays;
            
            // 2. Préfère les cartes proches de 7 (qui facilitent l'ouverture de séquences)
            int distanceFrom7 = std::abs(hand[idx].rank - 7);
            score += 0.2 * (7 - distanceFrom7);
            
            // 3. Facteur aléatoire pour briser les égalités
            score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
            
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return scoredMoves[0].first;
    }
    
    // Sélectionne le meilleur coup pour la phase de contrôle
    int selectControlMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // Dans la phase de contrôle, on joue sélectivement pour éviter d'aider les adversaires
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = 1.0; // Score de base
            
            // 1. Pénalise fortement les coups qui aident les adversaires
            if (willEnableOpponentPlays(idx, hand, tableLayout)) {
                score -= 1.5;
            }
            
            // 2. Bonus pour les coups qui nous donnent des options futures
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            score += 0.8 * futurePlays;
            
            // 3. Préfère maintenir un certain nombre d'options (pas trop, pas trop peu)
            if (futurePlays > 0 && futurePlays <= 2) {
                score += 0.5; // Nombre modéré d'options futures
            }
            
            // 4. Évite de se bloquer complètement
            if (futurePlays == 0 && hand.size() > 1) {
                score -= 3.0; // Forte pénalité
            }
            
            // 5. Facteur aléatoire pour briser les égalités
            score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
            
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return scoredMoves[0].first;
    }
    
    // Sélectionne le meilleur coup pour la phase finale
    int selectEndgameMove(
        const std::vector<int>& playableIndices,
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const 
    {
        // En fin de partie, on se concentre sur l'efficacité pour vider notre main
        std::vector<std::pair<int, double>> scoredMoves;
        
        for (int idx : playableIndices) {
            double score = 1.0; // Score de base
            const Card& card = hand[idx];
            
            // 1. Préfère les cartes extrêmes (A, 2, 3, J, Q, K) qui sont plus difficiles à jouer
            if (card.rank <= 3 || card.rank >= 11) {
                score += 1.5;
            }
            
            // 2. Vérifie si jouer cette carte nous permet de jouer une séquence
            int futurePlays = countFuturePlays(idx, hand, tableLayout);
            
            // 3. Très important d'avoir des options futures en fin de partie
            score += 2.0 * futurePlays;
            
            // 4. Évite absolument de se bloquer
            if (futurePlays == 0 && hand.size() > 1) {
                score -= 5.0; // Très forte pénalité
            }
            
            // 5. Facteur aléatoire pour briser les égalités
            score += std::uniform_real_distribution<>(0.0, 0.1)(const_cast<std::mt19937&>(rng));
            
            scoredMoves.push_back({idx, score});
        }
        
        // Trie par score (le plus élevé d'abord)
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                [](const auto& a, const auto& b) { return a.second > b.second; });
        
        return scoredMoves[0].first;
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
        return new sevens::TempoStrategy();
    }
}
#endif