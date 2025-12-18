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

    -- Precompiled header configuration (uses Engine's PCH)
    pchheader "pch.h"
    pchsource "../Engine/Source/pch.cpp"

    files {
        "Source/**.h",
        "Source/**.hpp",
        "Source/**.inl",
        "Source/**.cpp",
        "Source/**.cc",
        "../Engine/Source/pch.cpp",  -- Include PCH source file
    }

    includedirs {
        "Source",
        "../Engine/Source",
        "../Engine/" .. Dependencies.spdlog.IncludePath,
        "../Engine/" .. Dependencies.doctest.IncludePath,
        "../Engine/" .. Dependencies.glm.IncludePath,
        "../Engine/" .. Dependencies.json.IncludePath,
        "../Engine/" .. Dependencies.SDL3.IncludePath,
        "../Engine/" .. Dependencies.shaderc.IncludePath,
        "../Engine/" .. Dependencies.SPIRVCross.IncludePath,
        "../Engine/" .. Dependencies.msdfAtlasGen.IncludePath,
        "../Engine/" .. Dependencies.msdfAtlasGen.IncludePath .. "/msdfgen",  -- msdfgen headers needed by msdf-atlas-gen
        "../Engine/" .. Dependencies.Freetype.IncludePath,
        "../Engine/" .. Dependencies.OpenALSoft.IncludePath,
        "../Engine/" .. Dependencies.Libsndfile.IncludePath,
        "../Engine/" .. Dependencies.Libogg.IncludePath,
        "../Engine/" .. Dependencies.Libvorbis.IncludePath,
        "../Engine/" .. Dependencies.Minimp3.IncludePath,
        "../Engine/" .. Dependencies.Libflac.IncludePath,
        "../Engine/" .. Dependencies.Libopus.IncludePath,
        "../Engine/" .. Dependencies.Libopusenc.IncludePath,
        "../Engine/" .. Dependencies.Opusfile.IncludePath,
    }

    -- Link against Engine
    links { "Engine" }

    -- Enable engine entry point (provides main() function)
    defines { "SABORA_USE_ENTRY_POINT" }
    
    -- SDL3 static linking - must be linked directly since Engine is a static library
    defines { "SDL_STATIC" }
    
    -- OpenAL Soft static linking
    defines { "AL_LIBTYPE_STATIC" }

    -- PCH support for GCC/Clang (MSVC handles PCH automatically)
    filter { "toolset:gcc or toolset:clang" }
        buildoptions { "-include", "pch.h" }
    filter {}
    
    -- Library directory for SDL3 and shader libraries (relative to Sandbox, so we need to go up to Engine)
    libdirs { 
        "../Engine/" .. Dependencies.SDL3.LibraryPath,
        "../Engine/" .. Dependencies.shaderc.LibraryPath,
        "../Engine/" .. Dependencies.SPIRVCross.LibraryPath,
        "../Engine/" .. Dependencies.msdfAtlasGen.LibraryPath,
        "../Engine/" .. Dependencies.Freetype.LibraryPath,
        "../Engine/" .. Dependencies.OpenALSoft.LibraryPath,
        "../Engine/" .. Dependencies.Libsndfile.LibraryPath,
        "../Engine/" .. Dependencies.Libogg.LibraryPath,
        "../Engine/" .. Dependencies.Libvorbis.LibraryPath,
        "../Engine/" .. Dependencies.Libflac.LibraryPath,
        "../Engine/" .. Dependencies.Libopus.LibraryPath,
        "../Engine/" .. Dependencies.Libopusenc.LibraryPath,
        "../Engine/" .. Dependencies.Opusfile.LibraryPath,
        "../Engine/" .. Dependencies.Jolt.LibraryPath,
        "../Engine/" .. Dependencies.Box2D.LibraryPath,
    }

    filter "system:windows"
        -- Windows system libraries needed by SDL3 and OpenAL Soft
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid", "avrt" }
        buildoptions { "/utf-8" }
        defines { "FLAC__NO_DLL" }

    filter { "system:windows", "configurations:Debug" }
        links { 
            Dependencies.SDL3.Libraries.windows.Debug,
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
        links ( Dependencies.Libflac.Libraries.windows.Debug )
        links ( Dependencies.Libopus.Libraries.windows.Debug )
        links ( Dependencies.Libopusenc.Libraries.windows.Debug )
        links ( Dependencies.Opusfile.Libraries.windows.Debug )
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        links { 
            Dependencies.SDL3.Libraries.windows.Release,
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
        links ( Dependencies.Libflac.Libraries.windows.Release )
        links ( Dependencies.Libopus.Libraries.windows.Release )
        links ( Dependencies.Libopusenc.Libraries.windows.Release )
        links ( Dependencies.Opusfile.Libraries.windows.Release )
        runtime "Release"

    filter "system:macosx"
        -- macOS system libraries and frameworks needed by SDL3
        -- CoreFoundation is required by OpenAL Soft on macOS
        links { "CoreFoundation.framework",
                "CoreAudio.framework", 
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
        links { 
            Dependencies.SDL3.Libraries.macosx.Debug,
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
        links ( Dependencies.Libflac.Libraries.macosx.Debug )
        links ( Dependencies.Libopus.Libraries.macosx.Debug )
        links ( Dependencies.Libopusenc.Libraries.macosx.Debug )
        links ( Dependencies.Opusfile.Libraries.macosx.Debug )
        -- Link Jolt Physics static library
        links { Dependencies.Jolt.Libraries.macosx.Debug }

    filter { "system:macosx", "configurations:Release" }
        links { 
            Dependencies.SDL3.Libraries.macosx.Release,
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
        links ( Dependencies.Libflac.Libraries.macosx.Release )
        links ( Dependencies.Libopus.Libraries.macosx.Release )
        links ( Dependencies.Libopusenc.Libraries.macosx.Release )
        links ( Dependencies.Opusfile.Libraries.macosx.Release )
        -- Link Jolt Physics static library
        links { Dependencies.Jolt.Libraries.macosx.Release }

    filter { "system:linux", "configurations:Debug" }
        -- Prefer static libraries over shared libraries for our dependencies
        -- Add -L flags BEFORE -Wl,-Bstatic to ensure our library directories are searched first
        linkoptions {
            "-L../Engine/" .. Dependencies.Jolt.LibraryPath,
            "-Wl,-Bstatic"
        }
        -- Link SDL3 FIRST, then its dependencies
        -- On Linux, static libraries must be linked before their dependencies
        links { 
            Dependencies.SDL3.Libraries.linux.Debug,
            Dependencies.shaderc.Libraries.linux.Debug,
            Dependencies.OpenALSoft.Libraries.linux.Debug
        }
        links ( Dependencies.SPIRVCross.Libraries.linux.Debug )
        links ( Dependencies.msdfAtlasGen.Libraries.linux.Debug )
        links ( Dependencies.Freetype.Libraries.linux.Debug )
        -- Explicitly link libsndfile static library to avoid system library
        links ( Dependencies.Libsndfile.Libraries.linux.Debug )
        -- Use --start-group/--end-group for libvorbis libraries and libogg to handle dependencies
        -- Order: libvorbisfile/libvorbisenc depend on libvorbis, which depends on libogg
        -- On Unix linkers, dependencies must come after the libraries that need them
        linkoptions { "-Wl,--start-group" }
        links ( Dependencies.Libvorbis.LibrariesFile.linux.Debug )
        links ( Dependencies.Libvorbis.LibrariesEnc.linux.Debug )
        links ( Dependencies.Libvorbis.Libraries.linux.Debug )
        links ( Dependencies.Libogg.Libraries.linux.Debug )
        linkoptions { "-Wl,--end-group" }
        -- libFLAC can optionally use libogg, but we link it separately
        links ( Dependencies.Libflac.Libraries.linux.Debug )
        -- Link libopusenc first, then its dependency libopus
        links ( Dependencies.Libopusenc.Libraries.linux.Debug )
        links ( Dependencies.Libopus.Libraries.linux.Debug )
        -- opusfile depends on libopus and libogg, link it after them
        links ( Dependencies.Opusfile.Libraries.linux.Debug )
        -- Link Jolt Physics static library
        links { Dependencies.Jolt.Libraries.linux.Debug }
        -- Box2D built from source inside Engine on non-Windows
        -- Switch back to dynamic linking for system libraries
        linkoptions { "-Wl,-Bdynamic" }
        -- Now link all SDL3 dependencies
        links { 
            -- Core system
            "pthread", "dl", "m", "stdc++",
            -- X11 libraries (for X11 backend) - all X11 extensions
            -- Note: XShape functions are part of Xext, not a separate library
            "X11", "Xext", "Xrandr", "Xcursor", "Xfixes", "Xi", "Xinerama", 
            "Xxf86vm", "Xss", "Xtst", "Xrender",
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
        -- Prefer static libraries over shared libraries for our dependencies
        -- Add -L flags BEFORE -Wl,-Bstatic to ensure our library directories are searched first
        linkoptions {
            "-L../Engine/" .. Dependencies.Jolt.LibraryPath,
            "-Wl,-Bstatic"
        }
        -- Link SDL3 FIRST, then its dependencies
        -- On Linux, static libraries must be linked before their dependencies
        links { 
            Dependencies.SDL3.Libraries.linux.Release,
            Dependencies.shaderc.Libraries.linux.Release,
            Dependencies.OpenALSoft.Libraries.linux.Release
        }
        links ( Dependencies.SPIRVCross.Libraries.linux.Release )
        links ( Dependencies.msdfAtlasGen.Libraries.linux.Release )
        links ( Dependencies.Freetype.Libraries.linux.Release )
        -- Explicitly link libsndfile static library to avoid system library
        links ( Dependencies.Libsndfile.Libraries.linux.Release )
        -- Use --start-group/--end-group for libvorbis libraries and libogg to handle dependencies
        -- Order: libvorbisfile/libvorbisenc depend on libvorbis, which depends on libogg
        -- On Unix linkers, dependencies must come after the libraries that need them
        linkoptions { "-Wl,--start-group" }
        links ( Dependencies.Libvorbis.LibrariesFile.linux.Release )
        links ( Dependencies.Libvorbis.LibrariesEnc.linux.Release )
        links ( Dependencies.Libvorbis.Libraries.linux.Release )
        links ( Dependencies.Libogg.Libraries.linux.Release )
        linkoptions { "-Wl,--end-group" }
        -- libFLAC can optionally use libogg, but we link it separately
        links ( Dependencies.Libflac.Libraries.linux.Release )
        -- Link libopusenc first, then its dependency libopus
        links ( Dependencies.Libopusenc.Libraries.linux.Release )
        links ( Dependencies.Libopus.Libraries.linux.Release )
        -- opusfile depends on libopus and libogg, link it after them
        links ( Dependencies.Opusfile.Libraries.linux.Release )
        -- Link Jolt Physics static library
        links { Dependencies.Jolt.Libraries.linux.Release }
        -- Box2D built from source inside Engine on non-Windows
        -- Switch back to dynamic linking for system libraries
        linkoptions { "-Wl,-Bdynamic" }
        -- Now link all SDL3 dependencies
        links { 
            -- Core system
            "pthread", "dl", "m", "stdc++",
            -- X11 libraries (for X11 backend) - all X11 extensions
            -- Note: XShape functions are part of Xext, not a separate library
            "X11", "Xext", "Xrandr", "Xcursor", "Xfixes", "Xi", "Xinerama", 
            "Xxf86vm", "Xss", "Xtst", "Xrender",
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