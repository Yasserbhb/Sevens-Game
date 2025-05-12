#!/bin/bash

echo "Compiling Sevens Game..."
g++ -std=c++17 -Wall -Wextra -O3 \
MyCardParser.cpp \
MyGameParser.cpp \
MyGameMapper.cpp \
RandomStrategy.cpp \
GreedyStrategy.cpp \
main.cpp \
-o sevens_game -ldl -Wl,-rpath=.

echo ""
echo "Compiling RandomStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL RandomStrategy.cpp -o random_strategy.so

echo ""
echo "Compiling GreedyStrategy SO..."
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL GreedyStrategy.cpp -o greedy_strategy.so

echo ""
echo "Done!"