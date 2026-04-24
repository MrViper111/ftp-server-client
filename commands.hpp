#pragma once

#include <string>
#include <vector>

#include "types.hpp"
#include "files.hpp"

namespace Commands {

    struct Context {
        ClientData client;
        std::vector<ClientData> clients;
        std::vector<std::string> args;
        std::string reply;
        CacheManager *cache_manager = nullptr;
        bool exit_after = false;
        bool should_reply = true;
    };

    void quit(Context &context);
    void info(Context &context);
    void upload(Context &context);
    void download(Context &context);
    void uploads(Context &context);

}

