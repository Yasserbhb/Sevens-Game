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
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/RandomStrategy.cpp -o testing/random_strategy.so

echo ""
echo "Compiling GreedyStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/GreedyStrategy.cpp -o testing/greedy_strategy.so

echo ""
echo "Compiling StudentStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/FYM_Quest.cpp -o testing/FYM_Quest.so

echo "TESTING STRATS:  "
echo ""
echo "Compiling SequenceStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/SequenceStrategy.cpp -o testing/sequence_strategy.so

echo ""
echo "Compiling BlockingStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/BlockingStrategy.cpp -o testing/blocking_strategy.so

echo ""
echo "Compiling SevensRushStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/SevensRushStrategy.cpp -o testing/sevens_rush_strategy.so


echo ""
echo "Compiling BalanceStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/BalanceStrategy.cpp -o testing/balance_strategy.so


echo ""
echo "Done!"