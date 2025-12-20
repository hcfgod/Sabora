#include "pch.h"
#include "ShaderParser.h"
#include "Core/AsyncIO.h"
#include "Core/Log.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace Sabora
{
    //==========================================================================
    // Parse Methods
    //==========================================================================

    Result<std::unordered_map<ShaderStage, std::string>> ShaderParser::Parse(
        const std::string& source)
    {
        std::unordered_map<ShaderStage, std::string> shaderStages;
        
        if (source.empty())
        {
            return Result<std::unordered_map<ShaderStage, std::string>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Shader source is empty"
            );
        }

        std::istringstream stream(source);
        std::string line;
        ShaderStage currentStage = ShaderStage::Vertex;
        std::string currentSource;
        bool foundFirstType = false;

        while (std::getline(stream, line))
        {
            // Check for #type directive
            std::string trimmedLine = Trim(line);
            
            if (trimmedLine.find("#type") == 0)
            {
                // Save previous stage if we had one
                if (foundFirstType && !currentSource.empty())
                {
                    std::string trimmedSource = Trim(currentSource);
                    if (!trimmedSource.empty())
                    {
                        shaderStages[currentStage] = trimmedSource;
                    }
                }

                // Extract shader type
                size_t typeStart = trimmedLine.find_first_not_of(" \t", 5); // Skip "#type"
                if (typeStart != std::string::npos)
                {
                    std::string typeStr = trimmedLine.substr(typeStart);
                    typeStr = Trim(typeStr);
                    
                    currentStage = ParseShaderType(typeStr);
                    foundFirstType = true;
                    currentSource.clear();
                }
                else
                {
                    return Result<std::unordered_map<ShaderStage, std::string>>::Failure(
                        ErrorCode::CoreInvalidArgument,
                        "Invalid #type directive: missing shader type"
                    );
                }
            }
            else
            {
                // Add line to current shader source
                if (foundFirstType)
                {
                    currentSource += line + "\n";
                }
                else
                {
                    // Content before first #type directive - treat as vertex shader
                    if (currentSource.empty())
                    {
                        currentStage = ShaderStage::Vertex;
                        foundFirstType = true;
                    }
                    currentSource += line + "\n";
                }
            }
        }

        // Save last stage
        if (foundFirstType && !currentSource.empty())
        {
            std::string trimmedSource = Trim(currentSource);
            if (!trimmedSource.empty())
            {
                shaderStages[currentStage] = trimmedSource;
            }
        }

        if (shaderStages.empty())
        {
            return Result<std::unordered_map<ShaderStage, std::string>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "No valid shader stages found in source"
            );
        }

        return Result<std::unordered_map<ShaderStage, std::string>>::Success(std::move(shaderStages));
    }

    Result<std::unordered_map<ShaderStage, std::string>> ShaderParser::ParseFromFile(
        const std::filesystem::path& filePath)
    {
        // Read file
        auto readResult = AsyncIO::ReadTextFile(filePath);
        if (readResult.IsFailure())
        {
            return Result<std::unordered_map<ShaderStage, std::string>>::Failure(
                readResult.GetError().Code(),
                fmt::format("Failed to read shader file: {} - {}", filePath.string(), readResult.GetError().ToString())
            );
        }

        // Parse the source
        return Parse(readResult.Value());
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    ShaderStage ShaderParser::ParseShaderType(const std::string& typeStr)
    {
        // Convert to lowercase for case-insensitive matching
        std::string lowerType = typeStr;
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (lowerType == "vertex")
        {
            return ShaderStage::Vertex;
        }
        else if (lowerType == "fragment" || lowerType == "pixel")
        {
            return ShaderStage::Fragment;
        }
        else if (lowerType == "geometry")
        {
            return ShaderStage::Geometry;
        }
        else if (lowerType == "compute")
        {
            return ShaderStage::Compute;
        }
        else if (lowerType == "tessellationcontrol" || lowerType == "tesscontrol")
        {
            return ShaderStage::TessellationControl;
        }
        else if (lowerType == "tessellationevaluation" || lowerType == "tesseval")
        {
            return ShaderStage::TessellationEvaluation;
        }
        else
        {
            SB_CORE_WARN("Unknown shader type: '{}', defaulting to Vertex", typeStr);
            return ShaderStage::Vertex;
        }
    }

    std::string ShaderParser::Trim(const std::string& str)
    {
        if (str.empty())
        {
            return str;
        }

        size_t start = 0;
        size_t end = str.length() - 1;

        // Find first non-whitespace character
        while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start])))
        {
            ++start;
        }

        // Find last non-whitespace character
        while (end > start && std::isspace(static_cast<unsigned char>(str[end])))
        {
            --end;
        }

        if (start > end)
        {
            return "";
        }

        return str.substr(start, end - start + 1);
    }

} // namespace Sabora
