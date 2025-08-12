project "Engine"
    location "%{wks.basedir}/Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

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
        "Vendor/spdlog/include",
        "Vendor/doctest",
        "Vendor/glm",
        "Vendor/json/include",
        "Vendor/SDL/include",
    }

    libdirs { "%{wks.basedir}/Engine/Vendor/SDL/lib" }

    -- spdlog as header-only library
    defines { "SPDLOG_HEADER_ONLY", "SPDLOG_NO_EXCEPTIONS" }

    -- SDL3 static linking
    defines { "SDL_STATIC" }

    filter "system:windows"
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }
        defines { "_CRT_SECURE_NO_WARNINGS", "NOMINMAX" }

    filter { "system:windows", "configurations:Debug" }
        links { "SDL3-static-debug.lib" }
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        links { "SDL3-static-release.lib" }
        runtime "Release"

    filter "system:macosx"
        links { "libSDL3.a" }
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

    filter "system:linux"
        links { "libSDL3.a" }

    filter {}