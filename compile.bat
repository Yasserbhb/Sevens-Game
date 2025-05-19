@echo off
echo Compiling Sevens Game...
g++ -std=c++17 -Wall -Wextra -O3 ^
code_skeleton\MyCardParser.cpp ^
code_skeleton\MyGameParser.cpp ^
code_skeleton\MyGameMapper.cpp ^
code_skeleton\RandomStrategy.cpp ^
code_skeleton\GreedyStrategy.cpp ^
code_skeleton\main.cpp ^
-o testing\sevens_game.exe

echo.
echo Compiling RandomStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\RandomStrategy.cpp -o testing\random_strategy.dll

echo.
echo Compiling GreedyStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\GreedyStrategy.cpp -o testing\greedy_strategy.dll

echo.
echo Compiling StudentStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\FYM_Quest.cpp -o testing\FYM_Quest.dll

echo TESTING STRATS:  
echo.
echo Compiling SequenceStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\SequenceStrategy.cpp -o testing\sequence_strategy.dll

echo.
echo Compiling BlockingStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\BlockingStrategy.cpp -o testing\blocking_strategy.dll

echo.
echo Compiling SevensRushStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\SevensRushStrategy.cpp -o testing\sevens_rush_strategy.dll

echo.
echo Compiling BalanceStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\BalanceStrategy.cpp -o testing\balance_strategy.dll

echo.
echo Compiling HybridStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\HybridStrategy.cpp -o testing\hybrid_strategy.dll

echo.
echo Compiling EnhancedSequenceStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\EnhancedSequenceStrategy.cpp -o testing\enhancedsequence_strategy.dll

echo.
echo Compiling UltimateStrategy DLL...
g++ -std=c++17 -Wall -Wextra -O3 -shared -fPIC -DBUILD_SHARED_LIB code_skeleton\UltimateStrategy.cpp -o testing\ultimate_strategy.dll

echo.
echo Done!