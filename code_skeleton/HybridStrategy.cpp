// HybridStrategy.cpp
// Combinaison des points forts des stratégies Sequence, Blocking, Balance et SevensRush

#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>
#include <cmath>

namespace sevens {

/**
 * HybridStrategy
 *  - Séquences longues (Sequence/EnhancedSequence) ⟶ vider la main rapidement.
 *  - Équilibre des couleurs (Balance).
 *  - Gestion prudente des 7 + blocage (Blocking).
 *  - Accélération fin de partie + singletons (SevensRush).
 */
class HybridStrategy : public PlayerStrategy {
public:
    HybridStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        rng.seed(seed);
    }
    ~HybridStrategy() override = default;

    void initialize(uint64_t playerID) override {
        myID = playerID;
        roundTurn = 0;
    }

    int selectCardToPlay(const std::vector<Card>& hand,
                         const std::unordered_map<uint64_t,
                         std::unordered_map<uint64_t,bool>>& tableLayout) override {
        roundTurn++;

        // Trouver les cartes jouables
        std::vector<int> playableIdx = findPlayableCards(hand, tableLayout);
        if (playableIdx.empty()) return -1;
        if (playableIdx.size()==1) return playableIdx[0];

        // Stats sur la main
        auto suitCounts = countSuit(hand);
        auto suitImb    = suitImbalance(suitCounts, hand.size());
        int gamePhase   = phase(hand);

        int bestIdx = playableIdx[0];
        double bestScore = -1e9;

        for (int idx : playableIdx) {
            const Card& c = hand[idx];
            double score = 1.0;

            // 1. Longueur de séquence potentielle
            int seqLen = sequenceLength(idx, hand, tableLayout);
            score += SEQ_W * seqLen;

            // 2. Gestion des 7
            if (c.rank==7) {
                double sevenScore = SEVEN_W;
                if (suitCounts[c.suit]>=3 || suitImb[c.suit]>0) sevenScore += 1.0;
                else if (gamePhase<2) sevenScore -= 2.0; // garder pour bloquer
                score += sevenScore;
            }

            // 3. Équilibre des couleurs : jouer cartes des couleurs en excès
            score += BAL_W * suitImb[c.suit];

            // 4. Blocage : bonus si ne débloque pas de nouvelle extrémité
            if (!willEnableOpponents(c, tableLayout)) score += BLOCK_W;

            // 5. Coupures futures
            int future = futurePlays(idx, hand, tableLayout);
            score += FUTURE_W * future;

            // 6. Cartes extrêmes + singletons pour fin de partie
            if (isExtreme(c)) score += EXT_W;
            if (isSingleton(c, hand)) score += SINGLE_W;
            if (gamePhase==2) score += 0.5; // accélérer la sortie

            // 7. petit hasard
            score += std::uniform_real_distribution<>(0.0,0.15)(rng);

            if (score>bestScore) {bestScore=score; bestIdx=idx;}
        }
        return bestIdx;
    }

    void observeMove(uint64_t /*playerID*/, const Card& /*playedCard*/) override {}
    void observePass(uint64_t /*playerID*/) override {}
    std::string getName() const override {return "HybridStrategy";}

private:
    // Constantes de pondération
    static constexpr double SEQ_W = 2.0;
    static constexpr double SEVEN_W = 1.0;
    static constexpr double BAL_W = 0.6;
    static constexpr double BLOCK_W = 0.8;
    static constexpr double FUTURE_W = 0.4;
    static constexpr double EXT_W = 0.6;
    static constexpr double SINGLE_W = 0.7;

    uint64_t myID{};
    int roundTurn{};
    std::mt19937 rng;

    // ---------- Helpers ----------
    std::vector<int> findPlayableCards(const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t,bool>>& table) const {
        std::vector<int> out;
        for (int i=0;i<(int)hand.size();++i)
            if (isPlayable(hand[i], table)) out.push_back(i);
        return out;
    }

    bool isPlayable(const Card& card,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t,bool>>& table) const {
        int suit=card.suit, rank=card.rank;
        if (rank==7) {
            return !(table.count(suit)&&table.at(suit).count(rank)&&table.at(suit).at(rank));
        }
        bool high = (rank<13 && table.count(suit)&& table.at(suit).count(rank+1)&&table.at(suit).at(rank+1));
        bool low  = (rank>1  && table.count(suit)&& table.at(suit).count(rank-1)&&table.at(suit).at(rank-1));
        return high||low;
    }

    std::array<int,4> countSuit(const std::vector<Card>& hand) const {
        std::array<int,4> c{0,0,0,0};
        for (const auto& card: hand) c[card.suit]++;
        return c;
    }

    std::array<double,4> suitImbalance(const std::array<int,4>& cnt, size_t n) const {
        std::array<double,4> im{};
        double ideal = static_cast<double>(n)/4.0;
        for (int s=0;s<4;++s) im[s]=cnt[s]-ideal;
        return im;
    }

    int phase(const std::vector<Card>& hand) const {
        if (hand.size()>10) return 0; // early
        if (hand.size()>5)  return 1; // mid
        return 2; // late
    }

    int sequenceLength(int idx, const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t,bool>>& table) const {
        const Card& card = hand[idx];
        int len=1, suit=card.suit, rank=card.rank;
        auto sim = table; sim[suit][rank]=true;
        // Descendre
        for (int r=rank-1;r>=1;--r) {
            bool have=false;
            for (const auto& c: hand) if (c.suit==suit&&c.rank==r) {have=true; break;}
            if (!have) break;
            if (r==rank-1 || (sim[suit].count(r+1)&&sim[suit][r+1])) {sim[suit][r]=true; ++len;} else break;
        }
        // Monter
        for (int r=rank+1;r<=13;++r) {
            bool have=false;
            for (const auto& c: hand) if (c.suit==suit&&c.rank==r) {have=true; break;}
            if (!have) break;
            if (r==rank+1 || (sim[suit].count(r-1)&&sim[suit][r-1])) {sim[suit][r]=true; ++len;} else break;
        }
        return len;
    }

    bool willEnableOpponents(const Card& card,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t,bool>>& table) const {
        if (card.rank==7) return true; // ouvre deux côtés
        int suit=card.suit, rank=card.rank;
        bool lowOn= (rank>1  && table.count(suit)&&table.at(suit).count(rank-1)&&table.at(suit).at(rank-1));
        bool highOn=(rank<13 && table.count(suit)&&table.at(suit).count(rank+1)&&table.at(suit).at(rank+1));
        // Jouer une extrémité nouvelle ?
        return !(lowOn && highOn);
    }

    int futurePlays(int idx,const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t,bool>>& table) const {
        auto sim=table; const Card& pc=hand[idx]; sim[pc.suit][pc.rank]=true; int c=0;
        for (int i=0;i<(int)hand.size();++i){if(i==idx) continue; if(isPlayable(hand[i],sim)) ++c;}
        return c;
    }

    bool isExtreme(const Card& c) const {return c.rank<=3||c.rank>=11;}
    bool isSingleton(const Card& c, const std::vector<Card>& hand) const {
        int cnt=0; for(const auto& x:hand) if(x.suit==c.suit) ++cnt; return cnt==1;}
};

} // namespace sevens

// Loader pour la DLL/SO


#ifdef BUILD_SHARED_LIB
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::HybridStrategy();
}
#endif