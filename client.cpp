#include <arpa/inet.h>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>

#include "colors.hpp"
#include "util.hpp"

namespace ClientData {
    const std::string ADDRESS = "127.0.0.1";
    const int PORT = 8080;
    const std::string AUTH_FILE = "key.txt";
    const int BUFFER_SIZE = 1024;

    std::string get_auth_key() {
        std::ifstream file(AUTH_FILE);
        std::string contents;
        std::getline(file, contents);

        file.close();
        return contents;
    }
}

std::string prompt(std::string message) {
    std::string response;

    std::cout << "\n" << Colors::CYAN << message << "\n" << Colors::GRAY << "╰─ " << Colors::RESET;
    std::getline(std::cin, response);

    return response;
}

std::string send_message(int server_socket, std::string message) {
    char buffer[ClientData::BUFFER_SIZE];

    send(server_socket, message.c_str(), message.size(), 0);
    int bytes_received = recv(server_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        Print::error("Connection to server lost.");
        return "";
    }

    buffer[bytes_received] = '\0';
    return std::string(buffer);
}

int main() {
    std::cout << Colors::CYAN << "─────┤ FTP Client ├─────" << Colors::RESET << "\n";
    std::cout << Colors::GRAY << "You will be authenticated using " << ClientData::AUTH_FILE << ".\n" << Colors::RESET;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(ClientData::PORT);
    inet_pton(AF_INET, ClientData::ADDRESS.c_str(), &address.sin_addr);

    int connection = connect(server_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address));

    char buffer[ClientData::BUFFER_SIZE];
    ssize_t bytes_received;

    // Authenticate with the server

    std::string auth_key = ClientData::get_auth_key();
    std::string auth_response = send_message(server_socket, auth_key);

    if (auth_response == "Authentication successful!") {
        std::cout << Colors::GREEN << auth_response << Colors::RESET << "\n";
    } else {
        std::cout << Colors::RED << auth_response << Colors::RESET << "\n";

        close(server_socket);
        return EXIT_FAILURE;
    }

    // Select a nickname

    std::string response;
    do {
        std::string nickname = prompt("Enter a nickname for your client...");
        response = send_message(server_socket, nickname);
        if (response.starts_with("[ERROR]")) {
            std::cout << Colors::RED << response << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::GREEN << response << Colors::RESET << std::endl;
        }
    } while (!response.starts_with("Nickname accepted"));

    // Show the client list
    bytes_received = recv(server_socket, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_received] = '\0';
    std::cout << std::endl << buffer;
    std::cout << Colors::GRAY << "You can see the client list again by typing `clients`." << Colors::RESET << std::endl;


    while (true) {
        std::string message = prompt("Send to server...");
        send(server_socket, message.c_str(), message.size(), 0);

        bytes_received = recv(server_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            Print::error("Lost connection to server.");
            break;
        }
        buffer[bytes_received] = '\0';
        std::cout << "SERVER: " << buffer << std::endl;
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
