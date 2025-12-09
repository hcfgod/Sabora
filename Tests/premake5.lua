-- Test project configuration for Sabora Engine
-- Uses doctest for unit and integration testing

project "Tests"
    location "%{wks.basedir}/Tests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    
    -- Static runtime setting must match dependency build configuration
    local Dependencies = dofile("../Engine/Vendor/dependencies.lua")
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
        "Source/**.cpp",
        "Source/**.cc",
    }

    includedirs {
        "Source",
        "../Engine/Source",
        "../Engine/Vendor/doctest/doctest",
        "../Engine/Vendor/spdlog/include",
        "../Engine/Vendor/json/include",
    }

    -- Link against Engine library
    links { "Engine" }

    -- Note: DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN is defined in main.cpp, not here
    -- to avoid redefinition warnings

    filter "system:windows"
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }
        defines { "_CRT_SECURE_NO_WARNINGS", "NOMINMAX" }

    filter { "system:windows", "configurations:Debug" }
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        runtime "Release"

    filter "system:macosx"
        -- macOS system libraries if needed

    filter "system:linux"
        -- Linux system libraries needed by Engine/SDL3
        links { "pthread", "dl", "m" }

    filter {}

