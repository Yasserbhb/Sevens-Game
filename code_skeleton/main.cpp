// main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Include framework files
#include "MyGameMapper.hpp"
#include "RandomStrategy.hpp"
#include "GreedyStrategy.hpp"
#include "StrategyLoader.hpp"
// Windows-specific includes for dynamic loading

// For dynamic loading - platform-specific headers
#ifdef _WIN32
    // Windows systems
    #include <windows.h>
#else
    // Unix/Linux systems
    #include <dlfcn.h>
#endif

using namespace sevens;

// Function to load a strategy from a shared library (DLL on Windows)
std::shared_ptr<PlayerStrategy> loadStrategyFromLibrary(const std::string& libraryPath) {
#ifdef _WIN32
    // Windows version
    HMODULE handle = LoadLibraryA(libraryPath.c_str());
    if (!handle) {
        std::cerr << "Cannot open library: " << GetLastError() << std::endl;
        return nullptr;
    }
    
    // Load the createStrategy function
    typedef PlayerStrategy* (*CreateStrategyFn)();
    CreateStrategyFn createStrategy = (CreateStrategyFn)GetProcAddress(handle, "createStrategy");
    if (!createStrategy) {
        std::cerr << "Cannot load symbol 'createStrategy': " << GetLastError() << std::endl;
        FreeLibrary(handle);
        return nullptr;
    }
    
    // Create the strategy
    PlayerStrategy* strategy = createStrategy();
    if (!strategy) {
        std::cerr << "Failed to create strategy instance" << std::endl;
        FreeLibrary(handle);
        return nullptr;
    }
    
    // Wrap the raw pointer in a shared_ptr with custom deleter
    return std::shared_ptr<PlayerStrategy>(strategy, [handle](PlayerStrategy* p) {
        delete p;
        FreeLibrary(handle);
    });
#else
    // Linux/Unix version (same as before)
    void* handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot open library: " << dlerror() << std::endl;
        return nullptr;
    }
    
    // Clear any existing error
    dlerror();
    
    // Load the createStrategy function
    typedef PlayerStrategy* (*CreateStrategyFn)();
    CreateStrategyFn createStrategy = (CreateStrategyFn)dlsym(handle, "createStrategy");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'createStrategy': " << dlsym_error << std::endl;
        dlclose(handle);
        return nullptr;
    }
    
    // Create the strategy
    PlayerStrategy* strategy = createStrategy();
    if (!strategy) {
        std::cerr << "Failed to create strategy instance" << std::endl;
        dlclose(handle);
        return nullptr;
    }
    
    // Wrap the raw pointer in a shared_ptr with custom deleter
    return std::shared_ptr<PlayerStrategy>(strategy, [handle](PlayerStrategy* p) {
        delete p;
        dlclose(handle);
    });
#endif
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./sevens_game [mode] [optional libs...]\n";
        std::cout << "  Modes:\n";
        std::cout << "    internal - Run with default random strategies\n";
        std::cout << "    demo - Run with built-in strategies\n";
        std::cout << "    competition - Load strategies from .so/.dll files\n";
        return 1;
    }
    
    std::string mode = argv[1];
    MyGameMapper gameMapper;
    
    // Load the card data from file
    gameMapper.read_cards("cards.txt");
    
    // Initialize the game
    gameMapper.read_game("");
    
    if (mode == "internal") {
        std::cout << "Running in internal mode with default random strategies\n";
        
        // For internal mode, we'll use 4 players with RandomStrategy
        uint64_t numPlayers = 10;
        for (uint64_t i = 0; i < numPlayers; i++) {
            auto strategy = std::make_shared<RandomStrategy>();
            gameMapper.registerStrategy(i, strategy);
        }
        
        // Run the game
        auto results = gameMapper.compute_and_display_game(numPlayers);
        
        // Display results
        std::cout << "\nResults:\n";
        for (const auto& result : results) {
            std::cout << "Player " << result.first << " finished in position " << result.second << "\n";
        }
    }
    else if (mode == "demo") {
        std::cout << "Running in demo mode with built-in strategies\n";
        
        // For demo mode, we'll use 1 RandomStrategy and 1 GreedyStrategy
        auto randomStrat = std::make_shared<RandomStrategy>();
        auto greedyStrat = std::make_shared<GreedyStrategy>();
        
        gameMapper.registerStrategy(0, randomStrat);
        gameMapper.registerStrategy(1, greedyStrat);
        
        // Run the game
        auto results = gameMapper.compute_and_display_game(2);
        
        // Display results
        std::cout << "\nResults:\n";
        for (const auto& result : results) {
            uint64_t playerID = result.first;
            std::string strategyName;
            if (playerID == 0) {
                strategyName = randomStrat->getName();
            } else {
                strategyName = greedyStrat->getName();
            }
            std::cout << "Player " << playerID << " (" << strategyName << ") finished in position " << result.second << "\n";
        }
    }
    else if (mode == "competition") {
        if (argc < 3) {
            std::cout << "No strategy libraries provided for competition mode\n";
            #ifdef _WIN32
            std::cout << "Usage: sevens_game.exe competition strategy1.dll [strategy2.dll ...]\n";
            #else
            std::cout << "Usage: ./sevens_game competition strategy1.so [strategy2.so ...]\n";
            #endif
            return 1;
        }
        
        std::cout << "Running in competition mode with dynamic strategies\n";
        
        // Load strategies from shared libraries
        std::vector<std::shared_ptr<PlayerStrategy>> strategies;
        for (int i = 2; i < argc; i++) {
            std::shared_ptr<PlayerStrategy> strategy = loadStrategyFromLibrary(argv[i]);
            if (strategy) {
                strategies.push_back(strategy);
                std::cout << "Loaded strategy: " << strategy->getName() << " from " << argv[i] << "\n";
            }
        }
        
        if (strategies.empty()) {
            std::cout << "No valid strategies loaded\n";
            return 1;
        }
        
        // Register strategies with the game
        for (uint64_t i = 0; i < strategies.size(); i++) {
            gameMapper.registerStrategy(i, strategies[i]);
        }
        
        // Run the game
        auto results = gameMapper.compute_and_display_game(strategies.size());
        // Display results
        std::cout << "\nResults:\n";
        for (const auto& result : results) {
            uint64_t playerID = result.first;
            if (playerID < strategies.size()) {
                std::cout << "Player " << playerID << " (" << strategies[playerID]->getName() << ") finished in position " << result.second << "\n";
            }
        }
    }
    else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        return 1;
    }
    
    return 0;
}