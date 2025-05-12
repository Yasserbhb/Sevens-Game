# Sevens Card Game - Quick Start Guide

This README explains how to compile and run the Sevens card game with your StudentStrategy.

## Compiling the Game

### Windows

1. Open Command Prompt in the project directory
2. Run the compilation script:
```
compile.bat
```

This creates:
- `sevens_game.exe` (main game)
- `random_strategy.dll` (RandomStrategy)
- `greedy_strategy.dll` (GreedyStrategy)
- `student_strategy.dll` (StudentStrategy)

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

This creates:
- `sevens_game` (main game)
- `random_strategy.so` (RandomStrategy)
- `greedy_strategy.so` (GreedyStrategy)
- `student_strategy.so` (StudentStrategy)

## Running the Game

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
sevens_game.exe competition student_strategy.dll random_strategy.dll greedy_strategy.dll
```

**Linux**:
```
./sevens_game competition student_strategy.so random_strategy.so greedy_strategy.so
```

You can change the order or include only certain strategies:

**Windows**:
```
sevens_game.exe competition student_strategy.dll greedy_strategy.dll
```

**Linux**:
```
./sevens_game competition student_strategy.so greedy_strategy.so
```