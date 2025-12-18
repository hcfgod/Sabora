#include "pch.h"
#include "AsyncIO.h"
#include "Result.h"

#include <fstream>
#include <iterator>
#include <system_error>
#include <algorithm>
#include <limits>
#include <future>

namespace fs = std::filesystem;

namespace Sabora 
{
    //==========================================================================
    // Path Validation and Sanitization
    //==========================================================================

    Result<std::filesystem::path> AsyncIO::ValidateAndSanitizePath(
        const fs::path& path,
        bool allowAbsolute
    )
    {
        // Check for empty path
        if (path.empty())
        {
            return Result<fs::path>::Failure(
                ErrorCode::FileInvalidPath,
                "Path cannot be empty"
            );
        }

        // Check path length
        const std::string pathStr = path.string();
        if (pathStr.length() > MaxPathLength)
        {
            return Result<fs::path>::Failure(
                ErrorCode::FileInvalidPath,
                "Path exceeds maximum length of " + std::to_string(MaxPathLength) + " characters"
            );
        }

        // Check for parent directory references (path traversal attacks)
        const std::string pathString = path.string();
        if (pathString.find("..") != std::string::npos)
        {
            return Result<fs::path>::Failure(
                ErrorCode::FileInvalidPath,
                "Path contains parent directory references (..) which are not allowed"
            );
        }

        // Check for absolute paths if not allowed
        if (!allowAbsolute && path.is_absolute())
        {
            return Result<fs::path>::Failure(
                ErrorCode::FileInvalidPath,
                "Absolute paths are not allowed"
            );
        }

        // Normalize the path (remove redundant separators, resolve . references)
        // lexically_normal() doesn't require the path to exist
        try
        {
            fs::path normalizedPath = path.lexically_normal();
            
            // Additional validation: check for empty components after normalization
            if (normalizedPath.empty())
            {
                return Result<fs::path>::Failure(
                    ErrorCode::FileInvalidPath,
                    "Path normalizes to empty path"
                );
            }
            
            return Result<fs::path>::Success(std::move(normalizedPath));
        }
        catch (const fs::filesystem_error& e)
        {
            return Result<fs::path>::Failure(
                ErrorCode::FileInvalidPath,
                "Failed to normalize path: " + std::string(e.what())
            );
        }
        catch (...)
        {
            return Result<fs::path>::Failure(
                ErrorCode::FileInvalidPath,
                "Unknown error validating path"
            );
        }
    }

    //==========================================================================
    // Filesystem Helper Methods
    //==========================================================================

    bool AsyncIO::FileExists(const fs::path& path) noexcept
    {
        std::error_code ec;

        return fs::exists(path, ec) && fs::is_regular_file(path, ec);
    }

    bool AsyncIO::RemoveFile(const fs::path& path) noexcept 
    {
        std::error_code ec;

        if (!fs::exists(path, ec)) 
        {
            return true;
        }

        return fs::remove(path, ec);
    }

    bool AsyncIO::CreateDirectoriesFor(const fs::path& path) noexcept 
    {
        std::error_code ec;
        const fs::path parent = path.has_filename() ? path.parent_path() : path;

        if (parent.empty())
        {
            return true;
        }

        if (fs::exists(parent, ec)) 
        {
            return true;
        }

        return fs::create_directories(parent, ec);
    }

    std::vector<fs::path> AsyncIO::ListFiles(const fs::path& directory, bool recursive) 
    {
        std::vector<fs::path> files;
        std::error_code ec;

        if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec))
        {
            return files;
        }

        if (recursive) 
        {
            for (auto it = fs::recursive_directory_iterator(directory, ec); it != fs::recursive_directory_iterator(); ++it) 
            {
                if (it->is_regular_file()) 
                {
                    files.emplace_back(it->path());
                }
            }
        }
        else
        {
            for (auto it = fs::directory_iterator(directory, ec); it != fs::directory_iterator(); ++it)
            {
                if (it->is_regular_file()) 
                {
                    files.emplace_back(it->path());
                }
            }
        }

        return files;
    }

    Result<std::string> AsyncIO::ReadTextFile(const fs::path& path) 
    {
        // Validate path
        auto pathResult = ValidateAndSanitizePath(path, /*allowAbsolute*/ true);
        if (pathResult.IsFailure())
        {
            return Result<std::string>::Failure(pathResult.GetError());
        }
        const fs::path& sanitizedPath = pathResult.Value();

        std::ifstream in(sanitizedPath, std::ios::binary);

        if (!in) 
        {
            return Result<std::string>::Failure(
                ErrorCode::FileReadError,
                "Failed to open file for reading: " + sanitizedPath.string()
            );
        }

        std::string contents;
        in.seekg(0, std::ios::end);
        const auto size = in.tellg();
        
        if (size < 0)
        {
            return Result<std::string>::Failure(
                ErrorCode::FileReadError,
                "Failed to determine file size: " + sanitizedPath.string()
            );
        }
        
        // Bounds checking: ensure file size is within limits
        const auto sizeUnsigned = static_cast<std::size_t>(size);
        if (sizeUnsigned > MaxTextFileSize)
        {
            return Result<std::string>::Failure(
                ErrorCode::FileTooLarge,
                "File size (" + std::to_string(sizeUnsigned) + " bytes) exceeds maximum allowed size (" +
                std::to_string(MaxTextFileSize) + " bytes): " + sanitizedPath.string()
            );
        }
        
        contents.resize(sizeUnsigned);
        in.seekg(0, std::ios::beg);
        in.read(contents.data(), static_cast<std::streamsize>(contents.size()));

        if (in.fail() && !in.eof())
        {
            return Result<std::string>::Failure(
                ErrorCode::FileReadError,
                "Failed to read file contents: " + sanitizedPath.string()
            );
        }

        return Result<std::string>::Success(std::move(contents));
    }

    Result<std::vector<std::uint8_t>> AsyncIO::ReadBinaryFile(const fs::path& path) 
    {
        // Validate path
        auto pathResult = ValidateAndSanitizePath(path, /*allowAbsolute*/ true);
        if (pathResult.IsFailure())
        {
            return Result<std::vector<std::uint8_t>>::Failure(pathResult.GetError());
        }
        const fs::path& sanitizedPath = pathResult.Value();

        std::ifstream in(sanitizedPath, std::ios::binary);

        if (!in) 
        {
            return Result<std::vector<std::uint8_t>>::Failure(
                ErrorCode::FileReadError,
                "Failed to open file for reading: " + sanitizedPath.string()
            );
        }

        in.seekg(0, std::ios::end);
        const std::streamoff size = in.tellg();
        
        if (size < 0)
        {
            return Result<std::vector<std::uint8_t>>::Failure(
                ErrorCode::FileReadError,
                "Failed to determine file size: " + sanitizedPath.string()
            );
        }

        // Bounds checking: ensure file size is within limits
        const auto sizeUnsigned = static_cast<std::size_t>(size);
        if (sizeUnsigned > MaxBinaryFileSize)
        {
            return Result<std::vector<std::uint8_t>>::Failure(
                ErrorCode::FileTooLarge,
                "File size (" + std::to_string(sizeUnsigned) + " bytes) exceeds maximum allowed size (" +
                std::to_string(MaxBinaryFileSize) + " bytes): " + sanitizedPath.string()
            );
        }
        
        in.seekg(0, std::ios::beg);
        std::vector<std::uint8_t> data;
        data.resize(sizeUnsigned);

        if (size > 0) 
        {
            in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
            
            if (in.fail() && !in.eof())
            {
                return Result<std::vector<std::uint8_t>>::Failure(
                    ErrorCode::FileReadError,
                    "Failed to read file contents: " + sanitizedPath.string()
                );
            }
        }

        return Result<std::vector<std::uint8_t>>::Success(std::move(data));
    }

    Result<void> AsyncIO::WriteTextFile(const fs::path& path, std::string_view contents, bool createDirs)
    {
        if (createDirs) 
        {
            if (!CreateDirectoriesFor(path))
            {
                return Result<void>::Failure(
                    ErrorCode::FileWriteError,
                    "Failed to create directories for: " + path.string()
                );
            }
        }

        std::ofstream out(path, std::ios::binary | std::ios::trunc);

        if (!out) 
        {
            return Result<void>::Failure(
                ErrorCode::FileWriteError,
                "Failed to open file for writing: " + path.string()
            );
        }

        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        
        if (out.fail())
        {
            return Result<void>::Failure(
                ErrorCode::FileWriteError,
                "Failed to write file contents: " + path.string()
            );
        }

        return Result<void>::Success();
    }

    Result<void> AsyncIO::WriteBinaryFile(const fs::path& path, const std::vector<std::uint8_t>& data, bool createDirs) 
    {
        if (createDirs) 
        {
            if (!CreateDirectoriesFor(path))
            {
                return Result<void>::Failure(
                    ErrorCode::FileWriteError,
                    "Failed to create directories for: " + path.string()
                );
            }
        }

        std::ofstream out(path, std::ios::binary | std::ios::trunc);

        if (!out)
        {
            return Result<void>::Failure(
                ErrorCode::FileWriteError,
                "Failed to open file for writing: " + path.string()
            );
        }

        if (!data.empty()) 
        {
            out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
            
            if (out.fail())
            {
                return Result<void>::Failure(
                    ErrorCode::FileWriteError,
                    "Failed to write file contents: " + path.string()
                );
            }
        }

        return Result<void>::Success();
    }

    Result<nlohmann::json> AsyncIO::ReadJsonFile(const fs::path& path, bool validateStructure) 
    {
        // Validate path first
        auto pathResult = ValidateAndSanitizePath(path, /*allowAbsolute*/ true);
        if (pathResult.IsFailure())
    {
            return Result<nlohmann::json>::Failure(pathResult.GetError());
        }
        const fs::path& sanitizedPath = pathResult.Value();

        auto textResult = ReadTextFile(sanitizedPath);
        if (textResult.IsFailure())
        {
            return Result<nlohmann::json>::Failure(textResult.GetError());
        }

        // Check file size limit for JSON
        const std::string& text = textResult.Value();
        if (text.size() > MaxJsonFileSize)
        {
            return Result<nlohmann::json>::Failure(
                ErrorCode::FileTooLarge,
                "JSON file size (" + std::to_string(text.size()) + " bytes) exceeds maximum allowed size (" +
                std::to_string(MaxJsonFileSize) + " bytes): " + sanitizedPath.string()
            );
        }

        try
        {
            nlohmann::json json = nlohmann::json::parse(text);
            
            // Validate structure if requested
            if (validateStructure && !json.is_object())
            {
                return Result<nlohmann::json>::Failure(
                    ErrorCode::ValidationFailed,
                    "JSON structure validation failed: expected object, got " + std::string(json.type_name()) +
                    " in file: " + sanitizedPath.string()
                );
            }

            return Result<nlohmann::json>::Success(std::move(json));
        }
        catch (const nlohmann::json::parse_error& e)
        {
            return Result<nlohmann::json>::Failure(
                ErrorCode::FileInvalidFormat,
                "JSON parse error in file " + sanitizedPath.string() + ": " + e.what()
            );
        }
        catch (const nlohmann::json::exception& e)
        {
            return Result<nlohmann::json>::Failure(
                ErrorCode::FileInvalidFormat,
                "JSON error in file " + sanitizedPath.string() + ": " + e.what()
            );
        }
        catch (...)
        {
            return Result<nlohmann::json>::Failure(
                ErrorCode::FileInvalidFormat,
                "Unknown error parsing JSON file: " + sanitizedPath.string()
            );
        }
    }

    Result<void> AsyncIO::WriteJsonFile(const fs::path& path, const nlohmann::json& json, bool pretty, bool createDirs) 
    {
        if (createDirs) 
        {
            if (!CreateDirectoriesFor(path))
            {
                return Result<void>::Failure(
                    ErrorCode::FileWriteError,
                    "Failed to create directories for: " + path.string()
                );
            }
        }

        std::ofstream out(path, std::ios::binary | std::ios::trunc);

        if (!out) 
        {
            return Result<void>::Failure(
                ErrorCode::FileWriteError,
                "Failed to open file for writing: " + path.string()
            );
        }

        try
        {
            if (pretty)
            {
                out << json.dump(4);
            } 
            else 
            {
                out << json.dump();
            }
            
            if (out.fail())
            {
                return Result<void>::Failure(
                    ErrorCode::FileWriteError,
                    "Failed to write JSON to file: " + path.string()
                );
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            return Result<void>::Failure(
                ErrorCode::FileWriteError,
                "JSON serialization error: " + std::string(e.what())
            );
        }

        return Result<void>::Success();
    }

    std::future<Result<std::string>> AsyncIO::ReadTextFileAsync(fs::path path) 
    {
        try
        {
        return std::async(std::launch::async, [p = std::move(path)]()
            {
                try
        {
            return ReadTextFile(p);
                }
                catch (...)
                {
                    return Result<std::string>::Failure(
                        ErrorCode::FileReadError,
                        "Unhandled exception in async ReadTextFile: " + p.string()
                    );
                }
            });
        }
        catch (...)
        {
            // If std::async fails to create a thread, return a future with error
            std::promise<Result<std::string>> promise;
            promise.set_value(Result<std::string>::Failure(
                ErrorCode::CoreInitializationFailed,
                "Failed to create async task for ReadTextFile"
            ));
            return promise.get_future();
        }
    }

    std::future<Result<std::vector<std::uint8_t>>> AsyncIO::ReadBinaryFileAsync(fs::path path) 
    {
        try
        {
        return std::async(std::launch::async, [p = std::move(path)]()
            {
                try
        {
            return ReadBinaryFile(p);
                }
                catch (...)
                {
                    return Result<std::vector<std::uint8_t>>::Failure(
                        ErrorCode::FileReadError,
                        "Unhandled exception in async ReadBinaryFile: " + p.string()
                    );
                }
            });
        }
        catch (...)
        {
            std::promise<Result<std::vector<std::uint8_t>>> promise;
            promise.set_value(Result<std::vector<std::uint8_t>>::Failure(
                ErrorCode::CoreInitializationFailed,
                "Failed to create async task for ReadBinaryFile"
            ));
            return promise.get_future();
        }
    }

    std::future<Result<void>> AsyncIO::WriteTextFileAsync(fs::path path, std::string contents, bool createDirs) 
    {
        try
        {
        return std::async(std::launch::async, [p = std::move(path), c = std::move(contents), createDirs]() 
            {
                try
        {
            return WriteTextFile(p, c, createDirs);
                }
                catch (...)
                {
                    return Result<void>::Failure(
                        ErrorCode::FileWriteError,
                        "Unhandled exception in async WriteTextFile: " + p.string()
                    );
                }
            });
        }
        catch (...)
        {
            std::promise<Result<void>> promise;
            promise.set_value(Result<void>::Failure(
                ErrorCode::CoreInitializationFailed,
                "Failed to create async task for WriteTextFile"
            ));
            return promise.get_future();
        }
    }

    std::future<Result<void>> AsyncIO::WriteBinaryFileAsync(fs::path path, std::vector<std::uint8_t> data, bool createDirs) 
    {
        try
        {
        return std::async(std::launch::async, [p = std::move(path), d = std::move(data), createDirs]() 
            {
                try
        {
            return WriteBinaryFile(p, d, createDirs);
                }
                catch (...)
                {
                    return Result<void>::Failure(
                        ErrorCode::FileWriteError,
                        "Unhandled exception in async WriteBinaryFile: " + p.string()
                    );
                }
            });
        }
        catch (...)
        {
            std::promise<Result<void>> promise;
            promise.set_value(Result<void>::Failure(
                ErrorCode::CoreInitializationFailed,
                "Failed to create async task for WriteBinaryFile"
            ));
            return promise.get_future();
        }
    }

    std::future<Result<nlohmann::json>> AsyncIO::ReadJsonFileAsync(fs::path path) 
    {
        try
        {
        return std::async(std::launch::async, [p = std::move(path)]() 
            {
                try
        {
            return ReadJsonFile(p);
                }
                catch (...)
                {
                    return Result<nlohmann::json>::Failure(
                        ErrorCode::FileReadError,
                        "Unhandled exception in async ReadJsonFile: " + p.string()
                    );
                }
            });
        }
        catch (...)
        {
            std::promise<Result<nlohmann::json>> promise;
            promise.set_value(Result<nlohmann::json>::Failure(
                ErrorCode::CoreInitializationFailed,
                "Failed to create async task for ReadJsonFile"
            ));
            return promise.get_future();
        }
    }

    std::future<Result<void>> AsyncIO::WriteJsonFileAsync(fs::path path, nlohmann::json json, bool pretty, bool createDirs) 
    {
        try
        {
        return std::async(std::launch::async, [p = std::move(path), j = std::move(json), pretty, createDirs]() 
            {
                try
        {
            return WriteJsonFile(p, j, pretty, createDirs);
                }
                catch (...)
                {
                    return Result<void>::Failure(
                        ErrorCode::FileWriteError,
                        "Unhandled exception in async WriteJsonFile: " + p.string()
                    );
                }
            });
        }
        catch (...)
        {
            std::promise<Result<void>> promise;
            promise.set_value(Result<void>::Failure(
                ErrorCode::CoreInitializationFailed,
                "Failed to create async task for WriteJsonFile"
            ));
            return promise.get_future();
        }
    }
} // namespace Sabora