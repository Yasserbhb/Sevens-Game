#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <random>
#include <limits>

namespace sevens {

/* ---------- petite aide ---------- */
static const int MAX_PLAYERS = 8;   // suffisamment grand

class StudentStrategy : public PlayerStrategy
{
    using Table =
      std::unordered_map<uint64_t,
         std::unordered_map<uint64_t,bool>>;

    /* ------------- état interne ------------- */
    uint64_t           myID{};
    std::mt19937       rng{std::random_device{}()};
    /* suivi simplifié des tailles de main adverses */
    std::array<int,MAX_PLAYERS> oppHand{};   // -1 s'il n'existe pas
    int                nPlayers = 0;

    /* ---------- utilitaires table ---------- */
    static bool onTable(const Table& t,int s,int r)
    {
        auto it=t.find(s); if(it==t.end()) return false;
        const auto& inner=it->second;
        auto it2=inner.find(r);
        return it2!=inner.end() && it2->second;
    }
    static bool playable(const Card& c,const Table& t)
    {
        int s=c.suit, r=c.rank;
        if(onTable(t,s,r)) return false;
        if(r==7) return true;
        return (r>1  && onTable(t,s,r-1)) ||
               (r<13 && onTable(t,s,r+1));
    }

    /* longueur de chaîne que NOUS pouvons jouer après c */
    static int chainLen(const Card& c,
                        const std::vector<Card>& hand,
                        const Table& t)
    {
        int s=c.suit, r=c.rank, len=1;
        /* gauche */
        for(int rr=r-1; rr>=1; --rr){
            if(std::find_if(hand.begin(),hand.end(),
                 [&](const Card& o){return o.suit==s && o.rank==rr;})==hand.end())
                 break;
            if(rr<r-1 && !onTable(t,s,rr+1)) break;
            ++len;
        }
        /* droite */
        for(int rr=r+1; rr<=13; ++rr){
            if(std::find_if(hand.begin(),hand.end(),
                 [&](const Card& o){return o.suit==s && o.rank==rr;})==hand.end())
                 break;
            if(rr>r+1 && !onTable(t,s,rr-1)) break;
            ++len;
        }
        return len;
    }

    /* évalue si, une fois c joué, au moins une autre carte de notre main
       restera immédiatement jouable (pour éviter la “carte morte”) */
    static bool leavesFuturePlay(const Card& c,
                                 const std::vector<Card>& hand,
                                 const Table& t)
    {
        Table temp = t;
        temp[c.suit][c.rank] = true;          // simule pose
        for(const Card& o:hand){
            if(o.suit==c.suit && o.rank==c.rank) continue;
            if(playable(o,temp)) return true;
        }
        return false;
    }

public:
    /* ---------- interface ---------- */
    void initialize(uint64_t id) override
    {
        myID=id;
        for(int i=0;i<MAX_PLAYERS;++i) oppHand[i]=-1;
    }

    int selectCardToPlay(const std::vector<Card>& hand,
                         const Table& table) override
    {
        if(hand.empty()) return -1;

        /* estimation adversaires : init à 18 si inconnu (3 joueurs) */
        if(nPlayers==0){
            nPlayers=0;
            for(int i=0;i<MAX_PLAYERS;++i) if(oppHand[i]!=-2) ++nPlayers;
            if(nPlayers==0) nPlayers=3;
            int init= (52-int(hand.size()))/(nPlayers-1);
            for(int i=0;i<MAX_PLAYERS;++i)
                if(i!=static_cast<int>(myID)) oppHand[i]=init;
        }

        /* histogramme couleur */
        std::array<int,4> cnt{}; for(const Card& c:hand) ++cnt[c.suit];

        /* paramètres de score */
        const double W_UNBLOCK   = 2.0;
        const double W_BLOCK     = 1.5;
        const double W_CHAINLEN  = 1.0;
        const double W_HANDRED   = 4.0;   // fort en fin
        const int    PEN_SOLO7   = 6;
        const int    PEN_DEADEND = 8;

        int   bestIdx  = -1;
        double bestVal = -1e100;
        std::uniform_real_distribution<double> noise(-1e-3,1e-3);

        for(int i=0;i<(int)hand.size();++i){
            const Card& c=hand[i];
            if(!playable(c,table)) continue;

            /* pénalité 7 secondaire pauvre (≤2 cartes) */
            if(c.rank==7 && c.suit!=1 && cnt[c.suit]<=2) continue;

            /* composants */
            int unblock=0;
            for(const Card& o:hand)
                if(o.suit==c.suit && std::abs(o.rank-c.rank)==1 &&
                   !onTable(table,o.suit,o.rank))
                   ++unblock;

            /* bloque adversaire si voisin absent partout */
            double oppBlock=0.0;
            if(c.rank>1  && !onTable(table,c.suit,c.rank-1) &&
               std::find_if(hand.begin(),hand.end(),
                 [&](const Card& o){return o.suit==c.suit&&o.rank==c.rank-1;})==hand.end())
               oppBlock +=1.0;
            if(c.rank<13 && !onTable(table,c.suit,c.rank+1) &&
               std::find_if(hand.begin(),hand.end(),
                 [&](const Card& o){return o.suit==c.suit&&o.rank==c.rank+1;})==hand.end())
               oppBlock +=1.0;

            int len  = chainLen(c,hand,table);
            double handRed = 1.0/(1.0+hand.size());

            /* pénalités */
            int pen7 = (c.rank==7 && c.suit!=1 && cnt[c.suit]<4)?PEN_SOLO7:0;
            int dead = leavesFuturePlay(c,hand,table)?0:PEN_DEADEND;

            double score =
                W_UNBLOCK*unblock +
                W_BLOCK  *oppBlock +
                W_CHAINLEN*len +
                W_HANDRED*handRed -
                pen7 - dead +
                noise(rng);

            if(score>bestVal){ bestVal=score; bestIdx=i; }
        }

        /* fallback : ouvrir meilleur 7 si aucun coup choisi */
        if(bestIdx==-1){
            int best7=-1; int most=0;
            for(int i=0;i<(int)hand.size();++i){
                const Card& c=hand[i];
                if(c.rank!=7) continue;
                if(onTable(table,c.suit,7)) continue;
                if(cnt[c.suit]>most){ most=cnt[c.suit]; best7=i; }
            }
            bestIdx = best7; // peut rester -1 -> pass
        }

        return bestIdx;
    }

    /* suivi simple de la taille des mains adverses */
    void observeMove(uint64_t pid,const Card&) override
    {
        if(pid==myID) return;
        if(pid<MAX_PLAYERS && oppHand[pid]>0) --oppHand[pid];
    }
    void observePass(uint64_t pid) override
    {
        (void)pid; /* pas utilisé ici */
    }

    std::string getName() const override { return "HybridInferenceStrategy"; }
};

/* -------- factory -------- */
} // namespace sevens

#ifdef BUILDING_DLL
extern "C"{
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    sevens::PlayerStrategy* createStrategy(){ return new sevens::StudentStrategy(); }
}
#endif