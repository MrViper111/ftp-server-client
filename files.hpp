#pragma once

#include <string>
#include <vector>

#include "types.hpp"

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
