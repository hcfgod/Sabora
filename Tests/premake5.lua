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
        "../Engine/" .. Dependencies.Libogg.IncludePath,
        "../Engine/" .. Dependencies.Libvorbis.IncludePath,
    }

    -- Library directories for dependencies
    libdirs {
        "../Engine/" .. Dependencies.shaderc.LibraryPath,
        "../Engine/" .. Dependencies.SPIRVCross.LibraryPath,
        "../Engine/" .. Dependencies.msdfAtlasGen.LibraryPath,
        "../Engine/" .. Dependencies.Freetype.LibraryPath,
        "../Engine/" .. Dependencies.OpenALSoft.LibraryPath,
        "../Engine/" .. Dependencies.Libsndfile.LibraryPath,
        "../Engine/" .. Dependencies.Libogg.LibraryPath,
        "../Engine/" .. Dependencies.Libvorbis.LibraryPath,
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
        -- libogg must be linked before libvorbis (dependency)
        links ( Dependencies.Libogg.Libraries.windows.Debug )
        links ( Dependencies.Libvorbis.Libraries.windows.Debug )
        links ( Dependencies.Libvorbis.LibrariesFile.windows.Debug )
        links ( Dependencies.Libvorbis.LibrariesEnc.windows.Debug )
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
        -- libogg must be linked before libvorbis (dependency)
        links ( Dependencies.Libogg.Libraries.windows.Release )
        links ( Dependencies.Libvorbis.Libraries.windows.Release )
        links ( Dependencies.Libvorbis.LibrariesFile.windows.Release )
        links ( Dependencies.Libvorbis.LibrariesEnc.windows.Release )
        runtime "Release"

    filter "system:macosx"
        -- macOS system libraries if needed
        -- CoreFoundation is required by OpenAL Soft on macOS
        links { "CoreFoundation.framework", "CoreAudio.framework", "AudioToolbox.framework" }

    filter { "system:macosx", "configurations:Debug" }
        links { 
            Dependencies.shaderc.Libraries.macosx.Debug,
            Dependencies.OpenALSoft.Libraries.macosx.Debug
        }
        links ( Dependencies.SPIRVCross.Libraries.macosx.Debug )
        links ( Dependencies.msdfAtlasGen.Libraries.macosx.Debug )
        links ( Dependencies.Freetype.Libraries.macosx.Debug )
        links ( Dependencies.Libsndfile.Libraries.macosx.Debug )
        -- libogg must be linked before libvorbis (dependency)
        links ( Dependencies.Libogg.Libraries.macosx.Debug )
        links ( Dependencies.Libvorbis.Libraries.macosx.Debug )
        links ( Dependencies.Libvorbis.LibrariesFile.macosx.Debug )
        links ( Dependencies.Libvorbis.LibrariesEnc.macosx.Debug )

    filter { "system:macosx", "configurations:Release" }
        links { 
            Dependencies.shaderc.Libraries.macosx.Release,
            Dependencies.OpenALSoft.Libraries.macosx.Release
        }
        links ( Dependencies.SPIRVCross.Libraries.macosx.Release )
        links ( Dependencies.msdfAtlasGen.Libraries.macosx.Release )
        links ( Dependencies.Freetype.Libraries.macosx.Release )
        links ( Dependencies.Libsndfile.Libraries.macosx.Release )
        -- libogg must be linked before libvorbis (dependency)
        links ( Dependencies.Libogg.Libraries.macosx.Release )
        links ( Dependencies.Libvorbis.Libraries.macosx.Release )
        links ( Dependencies.Libvorbis.LibrariesFile.macosx.Release )
        links ( Dependencies.Libvorbis.LibrariesEnc.macosx.Release )

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
        -- Use -Wl,-Bstatic to force static linking for our vendor libraries
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
        -- libogg must be linked before libvorbis (dependency)
        -- Link using full path to ensure we get our static library, not system library
        -- The symlink libogg.a points to ogg-debug.a
        linkoptions { 
            "../Engine/" .. Dependencies.Libogg.LibraryPath .. "/libogg.a"
        }
        links ( Dependencies.Libvorbis.Libraries.linux.Debug )
        links ( Dependencies.Libvorbis.LibrariesFile.linux.Debug )
        links ( Dependencies.Libvorbis.LibrariesEnc.linux.Debug )
        -- Switch back to dynamic linking for system libraries
        linkoptions { "-Wl,-Bdynamic" }

    filter { "system:linux", "configurations:Release" }
        -- Prefer static libraries over shared libraries for our dependencies
        -- Use -Wl,-Bstatic to force static linking for our vendor libraries
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
        -- libogg must be linked before libvorbis (dependency)
        -- For Release, we need to update the symlink or use ogg-release.a directly
        -- Since libogg.a symlink points to ogg-debug.a, we use ogg-release.a directly
        linkoptions { 
            "../Engine/" .. Dependencies.Libogg.LibraryPath .. "/ogg-release.a"
        }
        links ( Dependencies.Libvorbis.Libraries.linux.Release )
        links ( Dependencies.Libvorbis.LibrariesFile.linux.Release )
        links ( Dependencies.Libvorbis.LibrariesEnc.linux.Release )
        -- Switch back to dynamic linking for system libraries
        linkoptions { "-Wl,-Bdynamic" }

    filter {}