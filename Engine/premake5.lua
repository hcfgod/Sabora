-- Load dependency configuration
-- Path is relative to this script's location (Engine/)
local Dependencies = dofile("Vendor/dependencies.lua")

project "Engine"
    location "%{wks.basedir}/Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    
    -- Static runtime setting must match dependency build configuration
    -- This is set globally in the root premake5.lua, but we ensure consistency here
    if Dependencies.SDL3.StaticRuntime then
        staticruntime "On"
    else
        staticruntime "Off"
    end

    targetdir ("%{wks.location}/bin/%{cfg.buildcfg}_%{cfg.platform}/%{prj.name}")
    objdir    ("%{wks.location}/obj/%{cfg.buildcfg}_%{cfg.platform}/%{prj.name}")

    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.inl",
        "Source/**.cpp",
        "Source/**.cc",
    }

    includedirs {
        "Source",
        Dependencies.spdlog.IncludePath,
        Dependencies.doctest.IncludePath,
        Dependencies.glm.IncludePath,
        Dependencies.json.IncludePath,
        Dependencies.SDL3.IncludePath,
    }

    -- Flexible library directory path based on configuration
    -- Supports different library locations per platform/configuration if needed
    -- Path is relative to Engine directory, so we use it directly
    libdirs { 
        Dependencies.SDL3.LibraryPath,
        -- Add platform/configuration-specific paths here if needed
        -- Example: Dependencies.SDL3.LibraryPath .. "/%{cfg.platform}/%{cfg.buildcfg}"
    }

    -- spdlog as header-only library
    defines { "SPDLOG_HEADER_ONLY", "SPDLOG_NO_EXCEPTIONS" }

    -- SDL3 static linking
    defines { "SDL_STATIC" }

    filter "system:windows"
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }
        defines { "_CRT_SECURE_NO_WARNINGS", "NOMINMAX" }

    filter { "system:windows", "configurations:Debug" }
        links { Dependencies.SDL3.Libraries.windows.Debug }
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        links { Dependencies.SDL3.Libraries.windows.Release }
        runtime "Release"

    filter "system:macosx"
        -- macOS system libraries needed by SDL3
        links { "CoreAudio.framework", 
                "CoreVideo.framework", 
                "IOKit.framework", 
                "Cocoa.framework", 
                "Carbon.framework", 
                "ForceFeedback.framework",
                "AVFoundation.framework", 
                "Metal.framework", 
                "QuartzCore.framework" }

    filter { "system:macosx", "configurations:Debug" }
        links { Dependencies.SDL3.Libraries.macosx.Debug }

    filter { "system:macosx", "configurations:Release" }
        links { Dependencies.SDL3.Libraries.macosx.Release }

    filter "system:linux"
        links { Dependencies.SDL3.Libraries.linux.Release }

    filter {}