#include "AsyncIO.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace Sabora::Engine {

bool AsyncIO::fileExists(const fs::path& path) noexcept {
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

bool AsyncIO::removeFile(const fs::path& path) noexcept {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        return true;
    }
    return fs::remove(path, ec);
}

bool AsyncIO::createDirectoriesFor(const fs::path& path) noexcept {
    std::error_code ec;
    const fs::path parent = path.has_filename() ? path.parent_path() : path;
    if (parent.empty()) {
        return true;
    }
    if (fs::exists(parent, ec)) {
        return true;
    }
    return fs::create_directories(parent, ec);
}

std::vector<fs::path> AsyncIO::listFiles(const fs::path& directory, bool recursive) {
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec)) {
        return files;
    }
    if (recursive) {
        for (auto it = fs::recursive_directory_iterator(directory, ec); it != fs::recursive_directory_iterator(); ++it) {
            if (it->is_regular_file()) {
                files.emplace_back(it->path());
            }
        }
    } else {
        for (auto it = fs::directory_iterator(directory, ec); it != fs::directory_iterator(); ++it) {
            if (it->is_regular_file()) {
                files.emplace_back(it->path());
            }
        }
    }
    return files;
}

std::string AsyncIO::readTextFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + path.string());
    }
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(static_cast<size_t>(in.tellg()));
    in.seekg(0, std::ios::beg);
    in.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    return contents;
}

std::vector<std::uint8_t> AsyncIO::readBinaryFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + path.string());
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> data;
    data.resize(static_cast<size_t>(size));
    if (size > 0) {
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    }
    return data;
}

void AsyncIO::writeTextFile(const fs::path& path, std::string_view contents, bool createDirs) {
    if (createDirs) {
        createDirectoriesFor(path);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }
    out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

void AsyncIO::writeBinaryFile(const fs::path& path,
                              const std::vector<std::uint8_t>& data,
                              bool createDirs) {
    if (createDirs) {
        createDirectoriesFor(path);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }
    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }
}

nlohmann::json AsyncIO::readJsonFile(const fs::path& path) {
    const std::string text = readTextFile(path);
    return nlohmann::json::parse(text);
}

void AsyncIO::writeJsonFile(const fs::path& path,
                            const nlohmann::json& json,
                            bool pretty,
                            bool createDirs) {
    if (createDirs) {
        createDirectoriesFor(path);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }
    if (pretty) {
        out << json.dump(4);
    } else {
        out << json.dump();
    }
}

std::future<std::string> AsyncIO::readTextFileAsync(fs::path path) {
    return std::async(std::launch::async, [p = std::move(path)]() {
        return readTextFile(p);
    });
}

std::future<std::vector<std::uint8_t>> AsyncIO::readBinaryFileAsync(fs::path path) {
    return std::async(std::launch::async, [p = std::move(path)]() {
        return readBinaryFile(p);
    });
}

std::future<void> AsyncIO::writeTextFileAsync(fs::path path,
                                              std::string contents,
                                              bool createDirs) {
    return std::async(std::launch::async, [p = std::move(path), c = std::move(contents), createDirs]() {
        writeTextFile(p, c, createDirs);
    });
}

std::future<void> AsyncIO::writeBinaryFileAsync(fs::path path,
                                                std::vector<std::uint8_t> data,
                                                bool createDirs) {
    return std::async(std::launch::async, [p = std::move(path), d = std::move(data), createDirs]() {
        writeBinaryFile(p, d, createDirs);
    });
}

std::future<nlohmann::json> AsyncIO::readJsonFileAsync(fs::path path) {
    return std::async(std::launch::async, [p = std::move(path)]() {
        return readJsonFile(p);
    });
}

std::future<void> AsyncIO::writeJsonFileAsync(fs::path path,
                                              nlohmann::json json,
                                              bool pretty,
                                              bool createDirs) {
    return std::async(std::launch::async, [p = std::move(path), j = std::move(json), pretty, createDirs]() {
        writeJsonFile(p, j, pretty, createDirs);
    });
}

} // namespace Sabora::Engine