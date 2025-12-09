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
        -- Linux system libraries needed by Engine/SDL3 and dependencies
        -- Note: SDL3 is linked in Engine, so we only need the system dependencies here
        -- Core system libraries
        links { "pthread", "dl", "m" }
        -- X11 libraries (for X11 backend) - all X11 extensions
        links { "X11", "Xext", "Xrandr", "Xcursor", "Xfixes", "Xi", "Xinerama", 
                "Xxf86vm", "Xss", "Xtst", "Xrender", "Xshape" }
        -- Wayland libraries (for Wayland backend)
        links { "wayland-client", "wayland-egl", "wayland-cursor", "xkbcommon" }
        -- Graphics libraries (EGL/GLES/DRM/GBM)
        links { "EGL", "GLESv2", "drm", "gbm" }
        -- Audio libraries (all backends SDL3 might use)
        links { "asound", "pulse", "jack", "pipewire-0.3" }
        -- USB and system libraries
        links { "usb-1.0", "dbus-1", "udev" }

    filter {}

