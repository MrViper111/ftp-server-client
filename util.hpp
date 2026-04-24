#pragma once

#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>


#include "colors.hpp"

namespace Print {

    inline void error(std::string message) {
        std::cout << Colors::RED << "[ERROR] " << message << Colors::RESET << "\n";
    }

    inline void info(std::string message) {
        std::cout << Colors::BLUE << "[INFO] " << Colors::RESET << message << "\n";
    }

}

namespace Strings {

    inline std::vector<std::string> split(const std::string &input, char delim) {
        std::vector<std::string> result;
        std::string current;

        for (char character : input) {
            if (character == delim) {
                result.push_back(current);
                current.clear();
            } else {
                current += character;
            }
        }

        result.push_back(current);
        return result;
    }

}

