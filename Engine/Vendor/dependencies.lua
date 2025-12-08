-- Dependency Configuration for Sabora Engine
-- This file tracks vendor library versions and build configurations
-- to ensure consistency across the build system

local Dependencies = {}

-- SDL3 Configuration
Dependencies.SDL3 = {
    -- Version string (update when pinning to specific version)
    Version = "3.x",
    
    -- Static runtime setting must match how SDL3 was built
    -- true = static runtime (/MT, /MTd), false = dynamic runtime (/MD, /MDd)
    StaticRuntime = false,
    
    -- Paths relative to Engine directory
    LibraryPath = "Vendor/SDL/lib",
    IncludePath = "Vendor/SDL/include",
    
    -- Library names per platform and configuration
    -- Note: On Unix systems (macOS/Linux), specify library name without 'lib' prefix and '.a' extension
    --       The linker will automatically add them (e.g., 'SDL3' becomes 'libSDL3.a')
    -- Note: On Windows, specify the full library name including extension
    Libraries = {
        windows = {
            Debug = "SDL3-static-debug.lib",
            Release = "SDL3-static-release.lib"
        },
        macosx = {
            Debug = "SDL3",
            Release = "SDL3"
        },
        linux = {
            Debug = "SDL3",
            Release = "SDL3"
        }
    }
}

-- spdlog Configuration (header-only library)
Dependencies.spdlog = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = true,
    -- Path relative to Engine directory
    IncludePath = "Vendor/spdlog/include"
}

-- doctest Configuration (header-only library)
Dependencies.doctest = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = true,
    -- Path relative to Engine directory
    IncludePath = "Vendor/doctest"
}

-- GLM Configuration (header-only library)
Dependencies.glm = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = true,
    -- Path relative to Engine directory
    IncludePath = "Vendor/glm"
}

-- nlohmann/json Configuration (header-only library)
Dependencies.json = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = true,
    -- Path relative to Engine directory
    IncludePath = "Vendor/json/include"
}

return Dependencies