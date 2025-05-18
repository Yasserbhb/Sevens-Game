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


echo Compiling StudentStrategy DLL...
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton/StudentTemplate.cpp -o testing/student_strategy.so

echo ""
echo "Done!"