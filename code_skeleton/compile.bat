@echo off

echo Compiling Sevens Game...
g++ -std=c++17 -Wall -Wextra -O3 -Wno-cast-function-type ^
MyCardParser.cpp ^
MyGameParser.cpp ^
MyGameMapper.cpp ^
RandomStrategy.cpp ^
GreedyStrategy.cpp ^
main.cpp ^
-o sevens_game.exe

echo.
echo Compiling RandomStrategy DLL...
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL RandomStrategy.cpp -o random_strategy.dll

echo.
echo Compiling GreedyStrategy DLL...
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL GreedyStrategy.cpp -o greedy_strategy.dll

echo.
echo Done!