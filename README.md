# Important Notes :

1. **Our Strategy and Code Files**: The `code_skeleton` directory contains all the code files, including our team's custom strategy (`FYM_Quest.cpp`), the test strategies (Random and Greedy), and all framework files. 

2. **Cross-Platform Support**:
   * The project has been compiled for both Linux (`.so` files, `sevens_game` executable) and Windows (`.dll` files, `sevens_game.exe` executable)
   * Our strategy (`FYM_Quest.so`/`.dll`) and the main executables are in the root directory
   * The test strategies (random and greedy) are in the `testing` directory

3. **Compilation**:
   * Use `compile.sh` (Linux) or `compile.bat` (Windows) in the root directory to recompile the project if needed
   * These scripts will compile:
     * The main game executable (`sevens_game`/`sevens_game.exe`) in the root
     * Our team strategy (`FYM_Quest.so`/`FYM_Quest.dll`) in the root
     * The test strategies in the testing directory

4. **Running the Game**:
   * From the root directory, run our strategy against the test strategies using:
     
     For Linux:
     ```
     ./sevens_game competition FYM_Quest.so testing/random_strategy.so testing/greedy_strategy.so
     ```
     
     For Windows:
     ```
     sevens_game.exe competition FYM_Quest.dll testing\random_strategy.dll testing\greedy_strategy.dll
     ```

# FYM_Quest Strategy

Our FYM_Quest strategy employs an adaptive, multi-factor decision system that consistently outperforms both random and greedy approaches in the Sevens card game.

## Strategy Flow

```mermaid
flowchart TD
    Start([FYM_Quest Strategy]) --> StateAnalysis["Game State Analysis<br><i>Input: Hand, Table Layout</i><br><i>Output: Game State Model</i>"]
    StateAnalysis --> FindCards["Find Playable Cards<br><i>Input: Hand, Table Layout</i><br><i>Output: List of Playable Cards</i>"]
    FindCards --> Decision{"Playable Cards?<br><i>Input: Playable Cards List</i>"}
    Decision -->|"None"| Pass["Pass Turn"]
    Decision -->|"One Card"| PlaySingle["Play That Card"]
    Decision -->|"Multiple Cards"| PhaseDetect["Game Phase Detection<br><i>Input: Hand Size</i><br><i>Output: Game Phase, Factor Weights</i>"]
    PhaseDetect --> ScoreCards["Card Scoring System<br><i>Inputs: Cards, Hand, Table Layout,<br>Game Phase, Factor Weights</i><br><i>Output: Score for Each Card</i>"]
    ScoreCards --> SelectCard["Select Highest-Scoring Card<br><i>Input: Card Scores</i><br><i>Output: Best Card to Play</i>"]
    SelectCard --> PlayCard["Play Selected Card"]
    
    subgraph "Game Phase Detection"
        direction LR
        Phase{Hand Size?} -->|">10 cards"| Early["Early Game<br>Priorities:<br>• Play 7s<br>• Build Sequences"]
        Phase -->|"6-10 cards"| Mid["Mid Game<br>Priorities:<br>• Extend Sequences<br>• Balance Offense/Defense"]
        Phase -->|"3-5 cards"| Late["Late Game<br>Priorities:<br>• Hand Reduction<br>• Play Extreme Cards"]
        Phase -->|"≤2 cards"| End["End Game<br>Priorities:<br>• Empty Hand ASAP"]
    end
    
    subgraph "Card Scoring Details"
        direction LR
        Scoring["For Each Playable Card:"] --> Seq["Sequence Analysis<br><i>Simulate chain of plays</i>"]
        Scoring --> Seven["Seven Strategy<br><i>Strategic 7s placement</i>"]
        Scoring --> Balance["Suit Balance<br><i>Play from overrepresented suits</i>"]
        Scoring --> Future["Future Plays<br><i>Create new play opportunities</i>"]
        Scoring --> Block["Blocking Potential<br><i>Prevent opponent plays</i>"]
        Scoring --> Extreme["Extreme Card Logic<br><i>Special handling for A,2,3,J,Q,K</i>"]
        
        Seq & Seven & Balance & Future & Block & Extreme --- Weights["Apply Phase-Based Weights"]
        Weights --> Calculate["Calculate Final Score"]
    end
    
    classDef default fill:#f9f9f9,stroke:#333,stroke-width:1px;
    classDef mainflow fill:#d4e6f1,stroke:#2874a6,stroke-width:2px;
    classDef decision fill:#fadbd8,stroke:#943126,stroke-width:1px;
    classDef terminal fill:#d5f5e3,stroke:#1e8449,stroke-width:1px;
    classDef phase fill:#fef9e7,stroke:#d4ac0d,stroke-width:1px;
    classDef scoring fill:#e8daef,stroke:#6c3483,stroke-width:1px;
    
    class Start,Pass,PlaySingle,PlayCard terminal;
    class StateAnalysis,FindCards,PhaseDetect,ScoreCards,SelectCard mainflow;
    class Decision decision;
    class Phase decision;
    class Early,Mid,Late,End phase;
    class Scoring,Seq,Seven,Balance,Future,Block,Extreme,Weights,Calculate scoring;