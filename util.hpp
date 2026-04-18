#pragma once

#include <iostream>
#include <string>

#include "colors.hpp"

namespace Print {

    inline void error(std::string message) {
        std::cout << Colors::RED << "[ERROR] " << message << Colors::RESET << "\n";
    }

    inline void info(std::string message) {
        std::cout << Colors::BLUE << "[INFO] " << Colors::RESET << message << "\n";
    }

}

