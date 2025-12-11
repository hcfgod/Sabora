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
        "../Engine/Source/Core",
        "../Engine/Vendor/doctest/doctest",
        "../Engine/Vendor/spdlog/include",
        "../Engine/Vendor/json/include",
        "../Engine/" .. Dependencies.shaderc.IncludePath,
        "../Engine/" .. Dependencies.SPIRVCross.IncludePath,
        "../Engine/" .. Dependencies.msdfAtlasGen.IncludePath,
        "../Engine/" .. Dependencies.msdfAtlasGen.IncludePath .. "/msdfgen",  -- msdfgen headers needed by msdf-atlas-gen
        "../Engine/" .. Dependencies.Freetype.IncludePath,
        "../Engine/" .. Dependencies.OpenALSoft.IncludePath,
        "../Engine/" .. Dependencies.Libsndfile.IncludePath,
    }

    -- Library directories for dependencies
    libdirs {
        "../Engine/" .. Dependencies.shaderc.LibraryPath,
        "../Engine/" .. Dependencies.SPIRVCross.LibraryPath,
        "../Engine/" .. Dependencies.msdfAtlasGen.LibraryPath,
        "../Engine/" .. Dependencies.Freetype.LibraryPath,
        "../Engine/" .. Dependencies.OpenALSoft.LibraryPath,
        "../Engine/" .. Dependencies.Libsndfile.LibraryPath,
    }

    -- Link against Engine library
    links { "Engine" }

    -- Note: DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN is defined in main.cpp, not here
    -- to avoid redefinition warnings

    filter "system:windows"
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }
        defines { "_CRT_SECURE_NO_WARNINGS", "NOMINMAX", "AL_LIBTYPE_STATIC" }

    filter { "system:windows", "configurations:Debug" }
        links { 
            Dependencies.shaderc.Libraries.windows.Debug,
            Dependencies.OpenALSoft.Libraries.windows.Debug
        }
        links ( Dependencies.SPIRVCross.Libraries.windows.Debug )
        links ( Dependencies.msdfAtlasGen.Libraries.windows.Debug )
        links ( Dependencies.Freetype.Libraries.windows.Debug )
        links ( Dependencies.Libsndfile.Libraries.windows.Debug )
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        links { 
            Dependencies.shaderc.Libraries.windows.Release,
            Dependencies.OpenALSoft.Libraries.windows.Release
        }
        links ( Dependencies.SPIRVCross.Libraries.windows.Release )
        links ( Dependencies.msdfAtlasGen.Libraries.windows.Release )
        links ( Dependencies.Freetype.Libraries.windows.Release )
        links ( Dependencies.Libsndfile.Libraries.windows.Release )
        runtime "Release"

    filter "system:macosx"
        -- macOS system libraries if needed
        links { "CoreAudio.framework", "AudioToolbox.framework" }

    filter { "system:macosx", "configurations:Debug" }
        -- Ensure our library directories are searched before system directories
        linkoptions { "-L%{wks.location}/../Engine/" .. Dependencies.Freetype.LibraryPath }
        links { 
            Dependencies.shaderc.Libraries.macosx.Debug,
            Dependencies.OpenALSoft.Libraries.macosx.Debug
        }
        links ( Dependencies.SPIRVCross.Libraries.macosx.Debug )
        links ( Dependencies.msdfAtlasGen.Libraries.macosx.Debug )
        -- Link freetype explicitly to avoid system library
        links ( Dependencies.Freetype.Libraries.macosx.Debug )
        links ( Dependencies.Libsndfile.Libraries.macosx.Debug )

    filter { "system:macosx", "configurations:Release" }
        -- Ensure our library directories are searched before system directories
        linkoptions { "-L%{wks.location}/../Engine/" .. Dependencies.Freetype.LibraryPath }
        links { 
            Dependencies.shaderc.Libraries.macosx.Release,
            Dependencies.OpenALSoft.Libraries.macosx.Release
        }
        links ( Dependencies.SPIRVCross.Libraries.macosx.Release )
        links ( Dependencies.msdfAtlasGen.Libraries.macosx.Release )
        -- Link freetype explicitly to avoid system library
        links ( Dependencies.Freetype.Libraries.macosx.Release )
        links ( Dependencies.Libsndfile.Libraries.macosx.Release )

    filter "system:linux"
        -- Prefer static libraries over shared libraries
        linkoptions { "-Wl,-Bstatic" }
        -- Linux system libraries needed by Engine/SDL3 and dependencies
        -- Note: SDL3 is linked in Engine, so we only need the system dependencies here
        -- Core system libraries
        links { "pthread", "dl", "m", "stdc++" }
        -- X11 libraries (for X11 backend) - all X11 extensions
        -- Note: XShape functions are part of Xext, not a separate library
        links { "X11", "Xext", "Xrandr", "Xcursor", "Xfixes", "Xi", "Xinerama", 
                "Xxf86vm", "Xss", "Xtst", "Xrender" }
        -- Wayland libraries (for Wayland backend)
        links { "wayland-client", "wayland-egl", "wayland-cursor", "xkbcommon" }
        -- Graphics libraries (EGL/GLES/DRM/GBM)
        links { "EGL", "GLESv2", "drm", "gbm" }
        -- Audio libraries (all backends SDL3 might use)
        links { "asound", "pulse", "jack", "pipewire-0.3" }
        -- USB and system libraries
        links { "usb-1.0", "dbus-1", "udev" }
        defines { "AL_LIBTYPE_STATIC" }

    filter { "system:linux", "configurations:Debug" }
        -- Prefer static libraries over shared libraries for our dependencies
        linkoptions { "-Wl,-Bstatic" }
        links { 
            Dependencies.shaderc.Libraries.linux.Debug,
            Dependencies.OpenALSoft.Libraries.linux.Debug
        }
        links ( Dependencies.SPIRVCross.Libraries.linux.Debug )
        links ( Dependencies.msdfAtlasGen.Libraries.linux.Debug )
        links ( Dependencies.Freetype.Libraries.linux.Debug )
        -- Explicitly link libsndfile static library to avoid system library
        links ( Dependencies.Libsndfile.Libraries.linux.Debug )
        -- Switch back to dynamic linking for system libraries
        linkoptions { "-Wl,-Bdynamic" }

    filter { "system:linux", "configurations:Release" }
        -- Prefer static libraries over shared libraries for our dependencies
        linkoptions { "-Wl,-Bstatic" }
        links { 
            Dependencies.shaderc.Libraries.linux.Release,
            Dependencies.OpenALSoft.Libraries.linux.Release
        }
        links ( Dependencies.SPIRVCross.Libraries.linux.Release )
        links ( Dependencies.msdfAtlasGen.Libraries.linux.Release )
        links ( Dependencies.Freetype.Libraries.linux.Release )
        -- Explicitly link libsndfile static library to avoid system library
        links ( Dependencies.Libsndfile.Libraries.linux.Release )
        -- Switch back to dynamic linking for system libraries
        linkoptions { "-Wl,-Bdynamic" }

    filter {}