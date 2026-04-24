#include <arpa/inet.h>
#include <fstream>
#include <csignal>
#include <cstdlib>
#include <iterator>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <format>
#include <iostream>
#include <thread>

#include "files.hpp"
#include "colors.hpp"
#include "util.hpp"
#include "types.hpp"
#include "commands.hpp"

namespace ServerData {
    const int PORT = 8080;
    const std::string AUTH_KEY = "piss";
    std::vector<ClientData> clients = {};
    std::mutex client_mutex;
    CacheManager &cache_manager = CacheManager::get_instance();

    std::optional<ClientData> get_client(std::string nickname) {
        for (ClientData &client : clients) {
            if (nickname == client.nickname) {
                return client;
            }
        }

        return std::nullopt; 
    }

    std::string get_clients_string() {
        std::string other_clients = Colors::GRAY + "Connected clients:\n" + Colors::RESET;
        for (ClientData &client : ServerData::clients) {
            other_clients += std::format("{}└{} {} ({})\n", Colors::GRAY, Colors::RESET, client.nickname, client.address);
        }

        return other_clients;
    }

    void add_client(ClientData client) {
        client_mutex.lock();
        clients.push_back(client);
        client_mutex.unlock();
    }

    void remove_client(int socket) {
        client_mutex.lock();
        
        std::erase_if(clients, [socket](const ClientData &data) {
            return data.socket == socket;
        });

        client_mutex.unlock();
    }
};

bool authenticate_client(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        close(client_socket);
        return false;
    }

    buffer[bytes_received] = '\0';

    std::string reply;
    bool authenticated;

    if (std::string(buffer) == ServerData::AUTH_KEY) {
        reply = "Authentication successful!";
        authenticated = true;
    } else {
        reply = "Authentication failed.";
        authenticated = false;
    }
    
    send(client_socket, reply.c_str(), reply.size(), 0);
    return authenticated;
}

std::string request_nickname(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        close(client_socket);
        return "";
    }

    buffer[bytes_received] = '\0';
    return buffer;
}

void handle_client(ClientData client) {
    char buffer[1024];

    while (true) {
        ssize_t bytes_received = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            Print::info(std::format("Client {} ({}) disconnected.", client.nickname, client.address));
            ServerData::remove_client(client.socket);
            close(client.socket);

            break;
        }

        buffer[bytes_received] = '\0';
        Print::info(std::format("[{}, {}] {}", client.nickname, client.address, buffer));

        std::string reply = "[ERROR] Invalid command, type `help` for a list of commands.";
        std::string buffer_str(buffer, bytes_received);
        std::vector<std::string> args = Strings::split(buffer_str, ' ');

        if (args.empty()) {
            send(client.socket, reply.c_str(), reply.size(), 0);
            continue;
        }
        Commands::Context context = {.client=client, .clients=ServerData::clients, .args=args,
            .cache_manager=&ServerData::cache_manager};
        context.reply = reply;

        if (args[0] == "info") {
            Commands::info(context);

        } else if (args[0] == "upload") {
            Commands::upload(context);

        } else if (args[0] == "download") {
            Commands::download(context);

        } else if (args[0] == "uploads") {
            Commands::uploads(context);

        } else if (args[0] == "quit") {
            Commands::quit(context);

            if (context.exit_after) {
                send(client.socket, context.reply.c_str(), context.reply.size(), 0);

                ServerData::remove_client(client.socket);
                close(client.socket);
                break;
            }
        }

        if (context.should_reply) {
            send(client.socket, context.reply.c_str(), context.reply.size(), 0);
        }
    }

    close(client.socket);
}

int main() {
    std::cout << Colors::CYAN << "─────┤ FTP Server ├─────" << Colors::RESET << "\n\n";
    Print::info("Starting server...");
    std::signal(SIGPIPE, SIG_IGN);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ServerData::PORT);

    int binded = bind(server_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (binded == -1) {
        Print::error(std::format("Failed to bind to port {}.", ServerData::PORT));
        close(server_socket);

        return EXIT_FAILURE;
    }

    listen(server_socket, 10);
    Print::info(std::format("Bound to port {}, listening for connections...", ServerData::PORT));

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        Print::info(std::format("Incoming connection from: {}", client_ip));

        // Authentication

        if (authenticate_client(client_socket)) {
            Print::info(std::format("Client ({}) has successfully authenticated!", client_ip));
        } else {
            Print::error(std::format("Client ({}) failed to authenticate.", client_ip));
            close(client_socket);

            continue;
        }

        // Get nickname
        
        std::string nickname;
        bool invalid_nickname = true;

        do {
            nickname = request_nickname(client_socket);

            if (nickname.empty()) {
                continue;
            }

            invalid_nickname = false;
            for (ClientData &client : ServerData::clients) {
                if (nickname == client.nickname) {
                    invalid_nickname = true;

                    std::string response = "[ERROR] This nickname is already in use, try again.";
                    send(client_socket, response.c_str(), response.size(), 0);

                    break;
                }
            }
        } while (invalid_nickname);

        // Create client data

        ClientData data = {.socket = client_socket, .address = client_ip, .nickname = nickname};
        Print::info(std::format("Created new client data: {{address={}, nickname={}}} [total={}]",
                client_ip, nickname, ServerData::clients.size() + 1));

        ServerData::add_client(data);

        const char* reply = "Nickname accepted, other clients will see you by this name.";
        send(client_socket, reply, strlen(reply), 0);

        std::string other_clients = ServerData::get_clients_string();
        send(client_socket, other_clients.c_str(), other_clients.size(), 0);

        std::thread(handle_client, data).detach();
    }

    close(server_socket);
    return EXIT_SUCCESS;
}

