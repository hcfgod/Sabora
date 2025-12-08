-- Root Premake script for Sabora

-- Load dependency configuration
local Dependencies = dofile("Engine/Vendor/dependencies.lua")

-- Configuration option: Static Runtime
-- This must match how vendor libraries (especially SDL3) were built
-- Set to true if SDL3 was built with static runtime (/MT, /MTd)
-- Set to false if SDL3 was built with dynamic runtime (/MD, /MDd)
-- Default: false (dynamic runtime) - matches typical CMake default
local UseStaticRuntime = Dependencies.SDL3.StaticRuntime

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
        if UseStaticRuntime then
            staticruntime "On"
        else
            staticruntime "Off"
        end

    filter "system:macosx"
        defines { "SABORA_PLATFORM_MACOS" }
        if UseStaticRuntime then
            staticruntime "On"
        else
            staticruntime "Off"
        end

    filter "system:linux"
        defines { "SABORA_PLATFORM_LINUX" }
        if UseStaticRuntime then
            staticruntime "On"
        else
            staticruntime "Off"
        end

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