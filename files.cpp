#include <vector>
#include <fstream>
#include <cstdint>
#include <filesystem>

#include "files.hpp"

CacheManager& CacheManager::get_instance() {
    static CacheManager instance;
    return instance;
}

void CacheManager::write_file(const std::string& filename, const std::vector<uint8_t> data, FileData file_data) {
    std::filesystem::create_directories(CACHE_PATH);
    std::ofstream file(std::filesystem::path(CACHE_PATH) / filename, std::ios::binary);

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    files.push_back(file_data);
}

