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

