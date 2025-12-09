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

    -- Link against Engine
    links { "Engine" }

    -- SDL3 static linking - must be linked directly since Engine is a static library
    defines { "SDL_STATIC" }
    
    -- Library directory for SDL3 (relative to Sandbox, so we need to go up to Engine)
    libdirs { 
        "../Engine/" .. Dependencies.SDL3.LibraryPath
    }

    filter "system:windows"
        -- Windows system libraries needed by SDL3
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }

    filter { "system:windows", "configurations:Debug" }
        links { Dependencies.SDL3.Libraries.windows.Debug }
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        links { Dependencies.SDL3.Libraries.windows.Release }
        runtime "Release"

    filter "system:macosx"
        -- macOS system libraries and frameworks needed by SDL3
        links { "CoreAudio.framework", 
                "CoreVideo.framework", 
                "IOKit.framework", 
                "Cocoa.framework", 
                "Carbon.framework", 
                "ForceFeedback.framework",
                "AVFoundation.framework", 
                "Metal.framework", 
                "QuartzCore.framework",
                "AudioToolbox.framework",
                "CoreHaptics.framework",
                "CoreMedia.framework",
                "GameController.framework",
                "UniformTypeIdentifiers.framework",
                "iconv" }
        -- Link libusb (installed via homebrew on macOS)
        libdirs { "/opt/homebrew/lib", "/usr/local/lib" }
        links { "usb-1.0" }

    filter { "system:macosx", "configurations:Debug" }
        links { Dependencies.SDL3.Libraries.macosx.Debug }

    filter { "system:macosx", "configurations:Release" }
        links { Dependencies.SDL3.Libraries.macosx.Release }

    filter { "system:linux", "configurations:Debug" }
        -- Link SDL3 FIRST, then its dependencies
        -- On Linux, static libraries must be linked before their dependencies
        links { Dependencies.SDL3.Libraries.linux.Debug }
        -- Now link all SDL3 dependencies
        links { 
            -- Core system
            "pthread", "dl", "m",
            -- X11 libraries (for X11 backend) - all X11 extensions
            "X11", "Xext", "Xrandr", "Xcursor", "Xfixes", "Xi", "Xinerama", 
            "Xxf86vm", "Xss", "Xtst", "Xrender", "Xshape",
            -- Wayland libraries (for Wayland backend)
            "wayland-client", "wayland-egl", "wayland-cursor", "xkbcommon",
            -- Graphics libraries (EGL/GLES/DRM/GBM)
            "EGL", "GLESv2", "drm", "gbm",
            -- Audio libraries (all backends SDL3 might use)
            "asound", "pulse", "jack", "pipewire-0.3",
            -- USB and system libraries
            "usb-1.0", "dbus-1", "udev"
        }

    filter { "system:linux", "configurations:Release" }
        -- Link SDL3 FIRST, then its dependencies
        -- On Linux, static libraries must be linked before their dependencies
        links { Dependencies.SDL3.Libraries.linux.Release }
        -- Now link all SDL3 dependencies
        links { 
            -- Core system
            "pthread", "dl", "m",
            -- X11 libraries (for X11 backend) - all X11 extensions
            "X11", "Xext", "Xrandr", "Xcursor", "Xfixes", "Xi", "Xinerama", 
            "Xxf86vm", "Xss", "Xtst", "Xrender", "Xshape",
            -- Wayland libraries (for Wayland backend)
            "wayland-client", "wayland-egl", "wayland-cursor", "xkbcommon",
            -- Graphics libraries (EGL/GLES/DRM/GBM)
            "EGL", "GLESv2", "drm", "gbm",
            -- Audio libraries (all backends SDL3 might use)
            "asound", "pulse", "jack", "pipewire-0.3",
            -- USB and system libraries
            "usb-1.0", "dbus-1", "udev"
        }

    filter {}