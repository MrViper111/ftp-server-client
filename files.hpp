#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

struct FileData {
    std::string filename;
    std::string recipient;
    std::string sender;
    std::optional<std::string> password;
};

class CacheManager {
public:
    static CacheManager& get_instance();

    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;

    const std::string CACHE_PATH = "cache";
    std::vector<FileData> files;

    void write_file(const std::string &filename, const std::vector<uint8_t> data, FileData file_data);

private:
    CacheManager() {}
};
