# Sevens Card Game - TeamFYM Strategy

## Project Structure
- `code_skeleton/` - All source code files including:
  - Game framework files (.cpp and .hpp)
  - RandomStrategy and GreedyStrategy implementations
  - Our TeamFYM_Strategy implementation
  - cards.txt file with deck configuration
- `testing/` - All compiled files and executables
- `compile.bat`/`compile.sh` - Compilation scripts

## Compiling the Game

### Windows
1. Open Command Prompt in the project directory
2. Run the compilation script:
```
compile.bat
```

This creates in the `testing` directory:
- `sevens_game.exe` (main game executable)
- `random_strategy.dll` (RandomStrategy library)
- `greedy_strategy.dll` (GreedyStrategy library)
- `teamFYM_strategy.dll` (Our TeamFYM_Strategy library)
- Copy of cards.txt for testing

### Linux
1. Open Terminal in the project directory
2. Make the compilation script executable (first time only):
```bash
chmod +x compile.sh
```
3. Run the compilation script:
```bash
./compile.sh
```

This creates in the `testing` directory:
- `sevens_game` (main game executable)
- `random_strategy.so` (RandomStrategy library)
- `greedy_strategy.so` (GreedyStrategy library)
- `teamFYM_strategy.so` (Our TeamFYM_Strategy library)
- Copy of cards.txt for testing

## Running the Game

First, navigate to the testing directory:

**Windows**:
```
cd testing
```

**Linux**:
```
cd testing
```

### Internal Mode (4 Random Players)
**Windows**:
```
sevens_game.exe internal
```

**Linux**:
```
./sevens_game internal
```

### Demo Mode (RandomStrategy vs GreedyStrategy)
**Windows**:
```
sevens_game.exe demo
```

**Linux**:
```
./sevens_game demo
```

### Competition Mode (Test Your Strategy)
**Windows**:
```
sevens_game.exe competition teamFYM_strategy.dll random_strategy.dll greedy_strategy.dll
```

**Linux**:
```
./sevens_game competition teamFYM_strategy.so random_strategy.so greedy_strategy.so
```

You can change the order or include only certain strategies:

**Windows**:
```
sevens_game.exe competition teamFYM_strategy.dll greedy_strategy.dll
```

**Linux**:
```
./sevens_game competition teamFYM_strategy.so greedy_strategy.so
```

## Strategy Description

TeamFYM_Strategy implements a sophisticated approach to the Sevens card game:

### Key Features:
1. **Multi-phase approach**:
   - Early game: Prioritizes playing 7s first to open lanes, then high-value cards
   - Mid-late game: Uses a scoring system for strategic card selection

2. **Card tracking**:
   - Maintains record of all played cards
   - Tracks available cards still in play
   - Monitors opponent moves and passes

3. **Strategic card selection**:
   - Scores potential plays based on multiple factors:
     - Base preference for higher cards (to empty hand faster)
     - Bonus for playing 7s (opening new play options)
     - Bonus for cards that enable more of your hand to be played
     - Adaptive penalties based on opponent behavior

4. **Opponent analysis**:
   - Records pass patterns to infer which cards opponents might not have
   - Adjusts strategy based on game progression

## Authors
- Yasser BOUHAI
- Fayçal CHEMLI
- Mohammed ADJIMI
## Notes for Submission
For final submission, only the TeamFYM_Strategy.cpp file (and any associated header files) need to be submitted. All testing infrastructure is only for development purposes.