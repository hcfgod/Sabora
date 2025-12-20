#include "pch.h"
#include "ShaderLoader.h"
#include "ShaderParser.h"
#include "ShaderCompiler.h"
#include "Renderer/Core/RendererTypes.h"
#include "Core/AsyncIO.h"
#include "Core/Log.h"
#include <algorithm>
#include <cctype>

namespace Sabora
{
    //==========================================================================
    // Constructor
    //==========================================================================

    ShaderLoader::ShaderLoader(uint32_t targetGLSLVersion)
        : m_TargetGLSLVersion(targetGLSLVersion)
    {
    }

    //==========================================================================
    // IAssetLoader Implementation
    //==========================================================================

    Result<std::unique_ptr<OpenGLShaderProgram>> ShaderLoader::Load(
        const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                ErrorCode::FileNotFound,
                fmt::format("Shader file not found: {}", path.string())
            );
        }

        // Get file extension
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // Determine format and load
        if (IsSingleFileFormat(extension))
        {
            return LoadSingleFile(path);
        }
        else
        {
            return LoadMultiFile(path);
        }
    }

    std::vector<std::string> ShaderLoader::GetSupportedExtensions() const
    {
        return { ".glsl", ".shader", ".vert", ".frag", ".geom", ".comp" };
    }

    //==========================================================================
    // Private Methods
    //==========================================================================

    Result<std::unique_ptr<OpenGLShaderProgram>> ShaderLoader::LoadSingleFile(
        const std::filesystem::path& path)
    {
        // Parse shader file to extract stages
        auto parseResult = ShaderParser::ParseFromFile(path);
        if (parseResult.IsFailure())
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                parseResult.GetError().Code(),
                fmt::format("Failed to parse shader file: {} - {}", path.string(), parseResult.GetError().ToString())
            );
        }

        auto shaderStages = parseResult.Value();

        // Compile each shader stage
        std::vector<std::unique_ptr<OpenGLShader>> compiledShaders;
        compiledShaders.reserve(shaderStages.size());

        for (const auto& [stage, source] : shaderStages)
        {
            // Compile using ShaderCompiler (GLSL -> SPIR-V -> GLSL)
            auto compileResult = ShaderCompiler::Compile(
                stage,
                source,
                path.string(),
                m_TargetGLSLVersion
            );

            if (compileResult.IsFailure())
            {
                return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                    compileResult.GetError().Code(),
                    fmt::format("Failed to compile shader stage {}: {} - {}", 
                        ShaderStageToString(stage), path.string(), compileResult.GetError().ToString())
                );
            }

            // Create OpenGLShader from compiled GLSL
            auto shaderResult = OpenGLShader::Create(stage, compileResult.Value().glslSource);
            if (shaderResult.IsFailure())
            {
                return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                    shaderResult.GetError().Code(),
                    fmt::format("Failed to create OpenGL shader for stage {}: {} - {}", 
                        ShaderStageToString(stage), path.string(), shaderResult.GetError().ToString())
                );
            }

            compiledShaders.push_back(std::move(shaderResult).Value());
        }

        // Link shader program
        auto programResult = OpenGLShaderProgram::Create(std::move(compiledShaders));
        if (programResult.IsFailure())
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                programResult.GetError().Code(),
                fmt::format("Failed to link shader program: {} - {}", path.string(), programResult.GetError().ToString())
            );
        }

        return Result<std::unique_ptr<OpenGLShaderProgram>>::Success(
            std::move(programResult).Value()
        );
    }

    Result<std::unique_ptr<OpenGLShaderProgram>> ShaderLoader::LoadMultiFile(
        const std::filesystem::path& path)
    {
        // Get base path (without extension)
        std::filesystem::path basePath = GetBasePath(path);

        // Determine which stage this file is
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // Note: initialStage is determined but not used - we try to load all common stages
        (void)GetStageFromExtension(extension);

        // Try to load common shader stages
        std::vector<std::pair<ShaderStage, std::filesystem::path>> stagesToLoad;
        
        // Always try vertex and fragment
        stagesToLoad.push_back({ ShaderStage::Vertex, basePath.string() + ".vert" });
        stagesToLoad.push_back({ ShaderStage::Fragment, basePath.string() + ".frag" });

        // Optionally try geometry and compute
        std::filesystem::path geomPath = basePath.string() + ".geom";
        if (std::filesystem::exists(geomPath))
        {
            stagesToLoad.push_back({ ShaderStage::Geometry, geomPath });
        }

        std::filesystem::path compPath = basePath.string() + ".comp";
        if (std::filesystem::exists(compPath))
        {
            stagesToLoad.push_back({ ShaderStage::Compute, compPath });
        }

        // Load and compile each stage
        std::vector<std::unique_ptr<OpenGLShader>> compiledShaders;
        compiledShaders.reserve(stagesToLoad.size());

        for (const auto& [stage, stagePath] : stagesToLoad)
        {
            if (!std::filesystem::exists(stagePath))
            {
                // Skip if file doesn't exist (except for vertex/fragment which are required)
                if (stage == ShaderStage::Vertex || stage == ShaderStage::Fragment)
                {
                    return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                        ErrorCode::FileNotFound,
                        fmt::format("Required shader file not found: {}", stagePath.string())
                    );
                }
                continue;
            }

            // Read shader source
            auto readResult = AsyncIO::ReadTextFile(stagePath);
            if (readResult.IsFailure())
            {
                return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                    readResult.GetError().Code(),
                    fmt::format("Failed to read shader file: {} - {}", stagePath.string(), readResult.GetError().ToString())
                );
            }

            std::string source = readResult.Value();

            // Compile using ShaderCompiler
            auto compileResult = ShaderCompiler::Compile(
                stage,
                source,
                stagePath.string(),
                m_TargetGLSLVersion
            );

            if (compileResult.IsFailure())
            {
                return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                    compileResult.GetError().Code(),
                    fmt::format("Failed to compile shader stage {}: {} - {}", 
                        ShaderStageToString(stage), stagePath.string(), compileResult.GetError().ToString())
                );
            }

            // Create OpenGLShader
            auto shaderResult = OpenGLShader::Create(stage, compileResult.Value().glslSource);
            if (shaderResult.IsFailure())
            {
                return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                    shaderResult.GetError().Code(),
                    fmt::format("Failed to create OpenGL shader for stage {}: {} - {}", 
                        ShaderStageToString(stage), stagePath.string(), shaderResult.GetError().ToString())
                );
            }

            compiledShaders.push_back(std::move(shaderResult).Value());
        }

        if (compiledShaders.empty())
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                ErrorCode::CoreInvalidArgument,
                fmt::format("No valid shader stages found for: {}", path.string())
            );
        }

        // Link shader program
        auto programResult = OpenGLShaderProgram::Create(std::move(compiledShaders));
        if (programResult.IsFailure())
        {
            return Result<std::unique_ptr<OpenGLShaderProgram>>::Failure(
                programResult.GetError().Code(),
                fmt::format("Failed to link shader program: {} - {}", path.string(), programResult.GetError().ToString())
            );
        }

        return Result<std::unique_ptr<OpenGLShaderProgram>>::Success(
            std::move(programResult).Value()
        );
    }

    bool ShaderLoader::IsSingleFileFormat(const std::string& extension)
    {
        std::string lowerExt = extension;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // Remove leading dot if present
        if (!lowerExt.empty() && lowerExt[0] == '.')
        {
            lowerExt = lowerExt.substr(1);
        }

        return lowerExt == "glsl" || lowerExt == "shader";
    }

    ShaderStage ShaderLoader::GetStageFromExtension(const std::string& extension)
    {
        std::string lowerExt = extension;
        std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
            [](unsigned char c) { return std::tolower(c); });

        // Remove leading dot if present
        if (!lowerExt.empty() && lowerExt[0] == '.')
        {
            lowerExt = lowerExt.substr(1);
        }

        if (lowerExt == "vert")
        {
            return ShaderStage::Vertex;
        }
        else if (lowerExt == "frag")
        {
            return ShaderStage::Fragment;
        }
        else if (lowerExt == "geom")
        {
            return ShaderStage::Geometry;
        }
        else if (lowerExt == "comp")
        {
            return ShaderStage::Compute;
        }

        return ShaderStage::Vertex; // Default
    }

    std::filesystem::path ShaderLoader::GetBasePath(const std::filesystem::path& path)
    {
        std::filesystem::path basePath = path;
        basePath.replace_extension("");
        return basePath;
    }

} // namespace Sabora
