#pragma once

#include <optional>
#include <string>

struct ClientData {
    int socket;
    std::string address;
    std::string nickname;
};

struct FileData {
    std::string filename;
    std::string recipient;
    std::string sender;
    std::optional<std::string> password;
};

