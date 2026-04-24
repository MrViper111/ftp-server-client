#include <string>
#include <format>
#include <arpa/inet.h>
#include <filesystem>
#include <fstream>

#include "commands.hpp"
#include "colors.hpp"
#include "util.hpp"

namespace Commands {

    void info(Context &context) {
        context.reply = std::format("nickname:{}, address:{}, socket:{}",
            context.client.nickname, context.client.address, context.client.socket
        );
    }

    void upload(Context &context) {
        if (context.args.size() != 3) {
            context.reply = "[ERROR] Invalid arguments! Usage: upload <file> <nickname>";
            return;
        }

        std::string filename = context.args[1];
        std::string sender = context.client.nickname;
        std::string recipient = context.args[2];

        ClientData client = context.client;

        std::string reply = std::format("SIG upload {} {}", filename, recipient);
        send(client.socket, reply.c_str(), reply.size(), 0);

        char buffer[1024]; // MAX UPLOAD / DOWNLOAD
        int bytes_recieved = recv(client.socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_recieved < 0) {
            Print::error("Client failed to upload file.");
            context.reply = "[ERROR] Upload failed.";
        } else {
            Print::info(std::format("Incoming file data from {}...", sender));
        }
        buffer[bytes_recieved] = '\0';

        FileData data = {.filename=filename, .recipient=recipient, .sender=sender};
        context.cache_manager->write_file(filename, std::vector<uint8_t>(buffer, buffer + bytes_recieved), data);

        Print::info(std::format("{} uploaded to {} with data: {{filename={}, recipient={}, sender={}}}",
                filename, "./cache", filename, recipient, sender));
        context.reply = "File upload complete! You can see your uploads by typing `uploads`.";
    }

    void download(Context &context) {
        if (context.args.size() != 2) {
            context.reply = "[ERROR] Invalid arguments! Usage: download <filename>";
            return;
        }

        std::string filename = context.args[1];
        ClientData client = context.client;
        std::vector<FileData> files = context.cache_manager->files;

        std::optional<FileData> target_file;
        for (FileData file : files) {
            if (file.filename == filename && client.nickname == file.recipient) {
                target_file = file;
            }
        }

        if (!target_file) {
            context.reply = "[ERROR] No such file found. Type `downloads` to see what files you can download.";
            return;
        }

        std::string signal = "SIG download " + filename;
        send(client.socket, signal.c_str(), signal.size(), 0);

        std::filesystem::path path = std::filesystem::path("cache") / target_file->filename;
        std::ifstream file(path, std::ios::binary);
        std::vector<uint8_t> file_bytes(std::istreambuf_iterator<char>(file), {});
        send(client.socket, file_bytes.data(), file_bytes.size(), 0);
    
        context.should_reply = false;
    }

    void uploads(Context &context) {
        std::vector<FileData> files = context.cache_manager->files;
        std::vector<FileData> client_files;
        std::stringstream stream;

        stream << std::format("Your file uploads for this session ({}):\n", client_files.size());
        for (FileData file : files) {
            if (file.sender == context.client.nickname) {
                client_files.push_back(file);
            }
        }

        if (client_files.empty()) {
            context.reply = Colors::RED + "You have uploaded no files this session.";
            return;
        }
        
        for (int i = 0; i < client_files.size(); i++) {
            FileData file = client_files[i];
            stream << Colors::GRAY << "- " << Colors::RESET << file.filename << " " << Colors::GRAY
                << "(for " << file.recipient << ")" << (i == client_files.size() ? "" : "\n");
        }

        context.reply = stream.str() + Colors::RESET;
    }

    void quit(Context &context) {
        context.exit_after = true;
        context.reply = "You are being disconnected from the server...";

        Print::info(std::format("{} ({}) is being disconnected...", context.client.nickname, context.client.address));
    }

}

