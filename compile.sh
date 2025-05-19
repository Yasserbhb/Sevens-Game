#!/bin/bash
echo "Compiling Sevens Game..."
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -pedantic-errors -O3 \
code_skeleton/MyCardParser.cpp \
code_skeleton/MyGameParser.cpp \
code_skeleton/MyGameMapper.cpp \
code_skeleton/RandomStrategy.cpp \
code_skeleton/GreedyStrategy.cpp \
code_skeleton/main.cpp \
-o sevens_game -ldl -Wl,-rpath=.

echo ""
echo "Compiling RandomStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/RandomStrategy.cpp -o testing/random_strategy.so

echo ""
echo "Compiling GreedyStrategy SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/GreedyStrategy.cpp -o testing/greedy_strategy.so

echo ""
echo "Compiling FYM_Quest SO..."
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton/FYM_Quest.cpp -o FYM_Quest.so


echo "Done!"