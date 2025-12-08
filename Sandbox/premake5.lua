-- Load dependency configuration
-- Path is relative to this script's location (Sandbox/)
local Dependencies = dofile("../Engine/Vendor/dependencies.lua")

project "Sandbox"
    location "%{wks.basedir}/Sandbox"
    kind "ConsoleApp"
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
        "../Engine/Source",
        "../Engine/" .. Dependencies.spdlog.IncludePath,
        "../Engine/" .. Dependencies.doctest.IncludePath,
        "../Engine/" .. Dependencies.glm.IncludePath,
        "../Engine/" .. Dependencies.json.IncludePath,
        "../Engine/" .. Dependencies.SDL3.IncludePath,
    }

    -- Link against Engine (which already contains SDL3)
    links { "Engine" }

    -- SDL3 static linking is handled by Engine project
    defines { "SDL_STATIC" }

    filter "system:windows"
        -- Windows system libraries needed by SDL3
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }

    filter { "system:windows", "configurations:Debug" }
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        runtime "Release"

    filter "system:macosx"

    filter "system:linux"

    filter {}