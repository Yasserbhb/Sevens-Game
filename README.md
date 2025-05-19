# Sevens Card Game - FYM Quest Strategy

## Team Members
- Faycal CHEMLI
- Yasser BOUHAI
- Mohamed LAJIMI

## Project Overview
This project implements a competitive strategy for the classic Sevens card game. Sevens is a trick-avoidance game where players strategically play cards according to specific rules:

- The game starts with the 7 of Diamonds on the table
- Players can only play cards that meet certain conditions:
  - 7s can be played only if not already on the table
  - Cards higher than 7 can be played if the card with rank-1 is on the table
  - Cards lower than 7 can be played if the card with rank+1 is on the table
- The first player to empty their hand wins the round
- Over multiple rounds, players accumulate cards left in their hands as penalty points
- The player with the lowest accumulated points wins the game

Our FYM_Quest strategy employs an adaptive, multi-factor decision system that consistently outperforms both random and greedy approaches by using sophisticated analysis and forward-looking simulation.

## Strategy Implementation

Our strategy follows a comprehensive decision process that analyzes the game state, identifies playable cards, and selects the optimal move based on multiple strategic factors that adapt as the game progresses.

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'fontFamily': 'arial', 'primaryTextColor': '#000000' }}}%%
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
    
    classDef default fill:#f9f9f9,stroke:#333,stroke-width:1px,color:#000;
    classDef mainflow fill:#d4e6f1,stroke:#2874a6,stroke-width:2px,color:#000;
    classDef decision fill:#fadbd8,stroke:#943126,stroke-width:1px,color:#000;
    classDef terminal fill:#d5f5e3,stroke:#1e8449,stroke-width:1px,color:#000;
    classDef phase fill:#fef9e7,stroke:#d4ac0d,stroke-width:1px,color:#000;
    classDef scoring fill:#e8daef,stroke:#6c3483,stroke-width:1px,color:#000;
    
    class Start,Pass,PlaySingle,PlayCard terminal;
    class StateAnalysis,FindCards,PhaseDetect,ScoreCards,SelectCard mainflow;
    class Decision decision;
    class Phase decision;
    class Early,Mid,Late,End phase;
    class Scoring,Seq,Seven,Balance,Future,Block,Extreme,Weights,Calculate scoring;
```

### Key Innovation: Sequence Analysis

A key innovation in our approach is the sequence analysis system that simulates potential card sequences to identify moves that enable the longest chain of consecutive plays. This forward-looking simulation gives our strategy a significant advantage over simpler approaches that only consider the immediate game state.

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'fontFamily': 'arial', 'primaryTextColor': '#000000' }}}%%
flowchart TD
    Start([Sequence Analysis]) --> Input["Inputs: Card to evaluate, Current hand, Table layout"]
    Input --> SimTable["Create Simulated Table with Selected Card Played"]
    SimTable --> MarkPlayed["Mark Card as Played in Simulation (Sequence Length = 1)"]
    MarkPlayed --> FindPlayable["Find All Cards That Would Become Playable After This Move"]
    FindPlayable --> CheckPlayable{"Any Newly Playable Cards?"}
    CheckPlayable -->|Yes| NextCard["Choose Next Card to Play in Simulation"]
    NextCard --> PlayNext["Add Card to Simulated Table<br>Increment Sequence Length"]
    PlayNext --> UpdatePlayable["Update List of Playable Cards"]
    UpdatePlayable --> CheckPlayable
    CheckPlayable -->|No| Result["Output: Maximum Sequence Length<br><i>Used in card scoring with weight based on game phase</i>"]
    
    subgraph "Example Visualization"
        direction LR
        Ex1["Initial Hand:<br>♠5, ♠6, ♥8, ♥9, ♣2, ♣4"] --- Ex2["Table Layout:<br>♦7, ♠7, ♥7"] --- Ex3["Evaluating ♠6:"]
        Ex3 --- Ex4["1. Play ♠6 (seq. length = 1)<br>Table: ♦7, ♠7, ♥7, ♠6"]
        Ex4 --- Ex5["2. ♠5 becomes playable<br>Play ♠5 (seq. length = 2)<br>Table: ♦7, ♠7, ♥7, ♠6, ♠5"]
        Ex5 --- Ex6["3. No more playable cards<br>Max sequence length = 2"]
    end
    
    classDef default fill:#f9f9f9,stroke:#333,stroke-width:1px,color:#000;
    classDef mainproc fill:#d4e6f1,stroke:#2874a6,stroke-width:2px,color:#000;
    classDef decision fill:#fadbd8,stroke:#943126,stroke-width:1px,color:#000;
    classDef terminal fill:#d5f5e3,stroke:#1e8449,stroke-width:1px,color:#000;
    classDef example fill:#fef9e7,stroke:#d4ac0d,stroke-width:1px,color:#000;
    
    class Start,Result terminal;
    class Input,SimTable,MarkPlayed,FindPlayable,NextCard,PlayNext,UpdatePlayable mainproc;
    class CheckPlayable decision;
    class Ex1,Ex2,Ex3,Ex4,Ex5,Ex6 example;
```

### Adaptive Card Scoring System

Each potential move is evaluated using six strategic factors, weighted according to the current game phase. This sophisticated scoring system enables our strategy to adapt its priorities as the game evolves, optimizing decision-making for each situation.

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'fontFamily': 'arial', 'primaryTextColor': '#000000' }}}%%
flowchart TD
    Start([Card Scoring Process]) --> Inputs["Inputs: Playable card, Current hand, Table layout, Game phase & weights"]
    
    Inputs --> Factors["Strategic Factors Evaluation:"]
    Factors --> Sequence["1. Sequence Analysis<br><i>Simulate playing card and subsequent cards</i>"]
    Factors --> Seven["2. Seven Strategy<br><i>Is it a 7? Is this suit over/under-represented?</i>"]
    Factors --> Balance["3. Suit Balance<br><i>Compare suit distribution to ideal (handSize ÷ 4)</i>"]
    Factors --> Future["4. Future Play Analysis<br><i>Count cards that would become playable</i>"]
    Factors --> Block["5. Blocking Assessment<br><i>Will this play avoid creating new endpoints?</i>"]
    Factors --> Extreme["6. Extreme Card Check<br><i>Is it A, 2, 3 or J, Q, K? Special handling if so</i>"]
    
    Sequence & Seven & Balance & Future & Block & Extreme --> WeightApply["Apply Phase-Based Weights to Each Factor"]
    
    WeightApply --> Formula["Final Score Formula:<br>score = baseScore + seqScore + sevenScore + balanceScore +<br>futureScore + blockScore + extremeScore + (small random factor)"]
    
    Formula --> Result["Output: Final Card Score<br><i>Higher score = better move</i>"]
    
    subgraph "Phase-Based Weight Adjustments"
        direction LR
        PhaseTitle["Game Phase Weight Examples"] ---
        Early["Early Game (>10 cards):<br>• Sequence: ×2.5<br>• Sevens: ×1.8<br>• Suit Balance: ×1.5<br>• Future Plays: ×1.0<br>• Blocking: ×0.5<br>• Extreme Cards: ×0.7"] ---
        Mid["Mid Game (6-10 cards):<br>• Sequence: ×2.5<br>• Sevens: ×1.5<br>• Suit Balance: ×1.2<br>• Future Plays: ×1.2<br>• Blocking: ×0.8<br>• Extreme Cards: ×1.0"] ---
        Late["Late Game (3-5 cards):<br>• Sequence: ×2.2<br>• Sevens: ×1.0<br>• Suit Balance: ×0.8<br>• Future Plays: ×1.8<br>• Blocking: ×1.0<br>• Extreme Cards: ×2.0"] ---
        End["End Game (≤2 cards):<br>• Sequence: ×1.5<br>• Sevens: ×0.5<br>• Suit Balance: ×0.3<br>• Future Plays: ×2.5<br>• Blocking: ×0.3<br>• Extreme Cards: ×2.0"]
    end
    
    classDef default fill:#f9f9f9,stroke:#333,stroke-width:1px,color:#000;
    classDef mainproc fill:#d4e6f1,stroke:#2874a6,stroke-width:2px,color:#000;
    classDef factor fill:#e8daef,stroke:#6c3483,stroke-width:1px,color:#000;
    classDef formula fill:#f5cba7,stroke:#a04000,stroke-width:1px,color:#000;
    classDef terminal fill:#d5f5e3,stroke:#1e8449,stroke-width:1px,color:#000;
    classDef phase fill:#fef9e7,stroke:#d4ac0d,stroke-width:1px,color:#000;
    
    class Start,Result terminal;
    class Inputs,Factors,WeightApply mainproc;
    class Sequence,Seven,Balance,Future,Block,Extreme factor;
    class Formula formula;
    class PhaseTitle,Early,Mid,Late,End phase;
```

## Strategy Justification

We chose this approach for several reasons:

1. **Forward-Looking Planning**: By simulating potential sequences of plays, our strategy can identify moves that might not seem optimal in the immediate term but lead to better long-term outcomes.

2. **Adaptive Gameplay**: Different phases of the game require different strategic priorities. Our phase-based weight adjustment system ensures the strategy shifts its focus appropriately as the game progresses.

3. **Multi-Factor Analysis**: Rather than relying on a single heuristic, our strategy combines multiple strategic factors to make more balanced decisions that consider various aspects of gameplay.

4. **Opponent Awareness**: The blocking potential assessment allows our strategy to play defensively in multiplayer games, preventing opponents from emptying their hands quickly.

5. **Suit Balance Optimization**: By considering the distribution of suits in hand, our strategy can prioritize playing from overrepresented suits, increasing the chances of emptying the hand efficiently.

## Sample Performance

![Game Performance Sample](test.mp4)

Our FYM_Quest strategy consistently outperforms both random and greedy approaches across multiple test games, demonstrating its effectiveness in various game scenarios.

## Important Notes

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

## Limitations and Future Improvements

While our strategy performs well, there are several areas for potential improvement:

1. **Enhanced Opponent Modeling**: More sophisticated tracking of opponent play patterns could improve defensive play.

2. **Dynamic Weight Adjustment**: The current phase-based weight system could be extended to adjust weights based on observed gameplay patterns.

3. **Machine Learning Integration**: A future version could use reinforcement learning to optimize the weights for different game situations.

4. **Performance Optimization**: The sequence analysis could be optimized to handle longer sequences without significant computational overhead.

## Acknowledgements

We would like to acknowledge:
- The card game framework provided by our instructors
- Research on optimal play in trick-taking card games that informed our strategy design
- Our C++ Advanced Programming course for providing the opportunity to work on this challenging project
