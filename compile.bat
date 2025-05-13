@echo off

echo Creating testing directory if it doesn't exist...
if not exist testing mkdir testing

echo Compiling Sevens Game...
g++ -std=c++17 -Wall -Wextra -O3 -Wno-cast-function-type ^
code_skeleton\MyCardParser.cpp ^
code_skeleton\MyGameParser.cpp ^
code_skeleton\MyGameMapper.cpp ^
code_skeleton\RandomStrategy.cpp ^
code_skeleton\GreedyStrategy.cpp ^
code_skeleton\main.cpp ^
-o testing\sevens_game.exe

echo.
echo Compiling RandomStrategy DLL...
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton\RandomStrategy.cpp -o testing\random_strategy.dll

echo.
echo Compiling GreedyStrategy DLL...
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton\GreedyStrategy.cpp -o testing\greedy_strategy.dll

echo.
echo Compiling StudentStrategy DLL...
g++ -std=c++17 -Wall -Wextra -shared -fPIC -DBUILDING_DLL code_skeleton\StudentTemplate.cpp -o testing\student_strategy.dll

echo.
echo Done!