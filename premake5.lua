-- Root Premake script for Sabora

workspace "Sabora"
    location "Build"
    startproject "Sandbox"

    configurations { "Debug", "Release" }
    platforms { "x64", "ARM64" }

    -- Map platforms to architectures
    filter "platforms:x64"
        architecture "x86_64"

    filter "platforms:ARM64"
        architecture "ARM64"

    -- Global settings per system
    filter "system:windows"
        systemversion "latest"
        defines { "SABORA_PLATFORM_WINDOWS", "_CRT_SECURE_NO_WARNINGS", "NOMINMAX" }
        staticruntime "On"

    filter "system:macosx"
        defines { "SABORA_PLATFORM_MACOS" }
        staticruntime "On"

    filter "system:linux"
        defines { "SABORA_PLATFORM_LINUX" }
        staticruntime "On"

    -- Configuration-specific
    filter "configurations:Debug"
        defines { "SABORA_DEBUG" }
        symbols "On"
        warnings "Extra"

    filter "configurations:Release"
        defines { "SABORA_RELEASE" }
        optimize "On"
        warnings "Extra"

    filter {}

include "Engine"
include "Sandbox"