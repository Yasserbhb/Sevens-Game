#pragma once

#include "PlayerStrategy.hpp"
#include <memory>
#include <string>

#include <stdexcept>
#include <iostream>


// Platform-specific headers
#ifdef _WIN32
    // Windows-specific includes
    #include <windows.h>
#else
    // Linux/Unix-specific includes
    #include <dlfcn.h>
#endif


namespace sevens {

/**
 * Utility class for loading player strategies from shared libraries.
 */
class StrategyLoader {
public:
    static std::shared_ptr<PlayerStrategy> loadFromLibrary(const std::string& libraryPath) {
    // Silence the warning
    (void)libraryPath;
    
    throw std::runtime_error("StrategyLoader::loadFromLibrary is not implemented in skeleton.");
    }
};

} // namespace sevens
