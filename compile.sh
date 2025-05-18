#!/bin/bash

echo "Compiling Sevens Game..."
g++ -std=c++17 -Wall -Wextra -O3 \
code_skeleton/MyCardParser.cpp \
code_skeleton/MyGameParser.cpp \
code_skeleton/MyGameMapper.cpp \
code_skeleton/RandomStrategy.cpp \
code_skeleton/GreedyStrategy.cpp \
code_skeleton/main.cpp \
-o testing/sevens_game -ldl -Wl,-rpath=.

echo ""
echo "Compiling RandomStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/RandomStrategy.cpp -o testing/random_strategy.so

echo ""
echo "Compiling GreedyStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/GreedyStrategy.cpp -o testing/greedy_strategy.so

echo ""
echo "Compiling StudentStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/FYM_Quest.cpp -o testing/FYM_Quest.so

echo "TESTING STRATS:  "
echo ""
echo "Compiling SequenceStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/SequenceStrategy.cpp -o testing/sequence_strategy.so

echo ""
echo "Compiling BlockingStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/BlockingStrategy.cpp -o testing/blocking_strategy.so

echo ""
echo "Compiling SevensRushStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/SevensRushStrategy.cpp -o testing/sevens_rush_strategy.so

echo ""
echo "Compiling AdaptiveStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/AdaptiveStrategy.cpp -o testing/adaptive_strategy.so

echo ""
echo "Compiling EndgameStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/EndgameStrategy.cpp -o testing/endgame_strategy.so

echo ""
echo "Compiling MirrorStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL MirrorStrategy.cpp -o testing/mirror_strategy.so

echo ""
echo "Compiling BalanceStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL BalanceStrategy.cpp -o testing/balance_strategy.so

echo ""
echo "Compiling TempoStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL TempoStrategy.cpp -o testing/tempo_strategy.so


echo ""
echo "Done!"