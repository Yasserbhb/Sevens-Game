#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <algorithm>
#include <random>
#include <chrono>
#include <string>

namespace sevens {

class HybridStrategy : public PlayerStrategy {
    using Table = std::unordered_map<uint64_t,std::unordered_map<uint64_t,bool>>;
public:
    HybridStrategy(){ rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count()); }
    void initialize(uint64_t pid) override { myID=pid; passStreak=0; playersSeen.clear(); playersSeen.insert(pid);}    

    int selectCardToPlay(const std::vector<Card>& hand,const Table& table) override {
        // collect playable
        std::vector<int> playable; for(int i=0;i<(int)hand.size();++i) if(isPlayable(hand[i],table)) playable.push_back(i);
        if(playable.empty()){ ++passStreak; return -1; }
        passStreak=0;

        // fast heuristic score for each playable
        std::vector<std::pair<int,double>> scored; scored.reserve(playable.size());
        auto suitCnt=countSuit(hand); auto imb=suitImbalance(suitCnt,hand.size());
        for(int idx:playable){ scored.push_back({idx,heuristic(idx,hand,table,suitCnt,imb)}); }
        std::sort(scored.begin(),scored.end(),[](auto&a,auto&b){return a.second>b.second;});

        // Monte‑Carlo light on top‑K
        const int K=std::min(4,(int)scored.size());
        int sims=25; // per move → ≤100 playouts total
        double bestVal=-1e9; int bestIdx=scored[0].first;
        for(int k=0;k<K;++k){ double val=0; for(int s=0;s<sims;++s){ val+=simulatePlayout(scored[k].first,hand,table);} val/=sims; if(val>bestVal){bestVal=val; bestIdx=scored[k].first;}}
        return bestIdx;
    }

    void observeMove(uint64_t pid,const Card&/*c*/) override {playersSeen.insert(pid);}    void observePass(uint64_t pid) override {playersSeen.insert(pid);}    std::string getName() const override {return "HybridMCTS";}
private:
    // weights
    static constexpr double SEQ_W=2.6,FUT_W=0.6,SEVEN_W=1.2,BAL_W=0.5,BLOCK_W=0.9,EXT_W=0.4,SING_W=0.4;

    uint64_t myID{}; std::mt19937 rng; int passStreak{0}; std::unordered_set<uint64_t>playersSeen;

    // ---------- Heuristic ----------
    double heuristic(int idx,const std::vector<Card>& h,const Table& tbl,const std::array<int,4>& suitCnt,const std::array<double,4>& imb) const{
        const Card& c=h[idx]; double s=0;
        s+=SEQ_W*deepSeq(idx,h,tbl,2);
        s+=FUT_W*futurePlayable(idx,h,tbl);
        if(c.rank==7){ double v=(suitCnt[c.suit]>=3?1:-1); s+=SEVEN_W*v;}
        s+=BAL_W*imb[c.suit];
        if(!opensNewEnd(c,tbl)) s+=BLOCK_W;
        if(isExtreme(c)) s+=EXT_W; if(isSingleton(c,h)) s+=SING_W;
        return s;
    }

    // ---------- Monte‑Carlo playout ----------
    double simulatePlayout(int idx,const std::vector<Card>& hand,const Table& table) const{
        // Copy state
        Table t=table; std::vector<Card> h=hand; Card first=h[idx]; t[first.suit][first.rank]=true; h.erase(h.begin()+idx);
        int moves=1;
        // random shuffle remaining for quick rollout
        std::shuffle(h.begin(),h.end(),rng);
        for(const Card& c:h){ if(isPlayable(c,t)){ t[c.suit][c.rank]=true; ++moves; }}
        return moves; // more moves we can play ⇒ better
    }

    // ---------- utils ----------
    bool isPlayable(const Card& cd,const Table& tbl) const{ auto it=tbl.find(cd.suit); if(cd.rank==7) return !(it!=tbl.end()&&it->second.count(7)&&it->second.at(7)); if(it==tbl.end()) return false; const auto&r=it->second; return (cd.rank<13&&r.count(cd.rank+1)&&r.at(cd.rank+1))||(cd.rank>1&&r.count(cd.rank-1)&&r.at(cd.rank-1));}
    bool opensNewEnd(const Card& c,const Table&tbl) const{ if(c.rank==7) return true; auto it=tbl.find(c.suit); if(it==tbl.end()) return true; const auto&r=it->second; bool lo=(c.rank>1&&r.count(c.rank-1)&&r.at(c.rank-1)); bool hi=(c.rank<13&&r.count(c.rank+1)&&r.at(c.rank+1)); return !(lo&&hi);}    
    std::array<int,4> countSuit(const std::vector<Card>& h) const{ std::array<int,4>cnt{0,0,0,0}; for(auto&c:h)cnt[c.suit]++; return cnt; }
    std::array<double,4> suitImbalance(const std::array<int,4>&cnt,size_t n) const{ std::array<double,4>im{}; double ideal=n/4.0; for(int i=0;i<4;++i) im[i]=cnt[i]-ideal; return im; }
    int deepSeq(int idx,const std::vector<Card>&h,const Table&tbl,int depth) const{ Table sim=tbl; const Card&st=h[idx]; sim[st.suit][st.rank]=true; int len=1; std::vector<Card> rem; for(int i=0;i<(int)h.size();++i) if(i!=idx) rem.push_back(h[i]); for(int d=0;d<depth;++d){ bool moved=false; for(size_t j=0;j<rem.size();++j){ if(isPlayable(rem[j],sim)){ sim[rem[j].suit][rem[j].rank]=true; rem.erase(rem.begin()+j); ++len; moved=true; break; }} if(!moved) break;} return len; }
    int futurePlayable(int idx,const std::vector<Card>&h,const Table&tbl) const{ Table sim=tbl; const Card&c=h[idx]; sim[c.suit][c.rank]=true; int cnt=0; for(int i=0;i<(int)h.size();++i) if(i!=idx&&isPlayable(h[i],sim)) ++cnt; return cnt; }
    bool isExtreme(const Card& c) const{return c.rank<=3||c.rank>=11;}
    bool isSingleton(const Card& c,const std::vector<Card>& h) const{ int k=0; for(auto&x:h) if(x.suit==c.suit) ++k; return k==1; }
};

}
#ifdef BUILD_SHARED_LIB
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::HybridStrategy();
}
#endif