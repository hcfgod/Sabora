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
    -- SDL3 is built with static runtime when SDL_STATIC=ON (which we use)
    StaticRuntime = true,
    
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

-- shaderc Configuration (static library - includes SPIRV-Tools and SPIRV-Headers)
Dependencies.shaderc = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = false,
    
    -- Paths relative to Engine directory
    LibraryPath = "Vendor/shaderc/lib",
    IncludePath = "Vendor/shaderc/include",
    
    -- Library names per platform and configuration
    -- Note: On Unix systems, specify library name without 'lib' prefix and '.a' extension
    Libraries = {
        windows = {
            Debug = "shaderc-debug.lib",
            Release = "shaderc-release.lib"
        },
        macosx = {
            Debug = "shaderc",
            Release = "shaderc"
        },
        linux = {
            Debug = "shaderc",
            Release = "shaderc"
        }
    }
}

-- SPIRV-Cross Configuration (static library)
Dependencies.SPIRVCross = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = false,
    
    -- Paths relative to Engine directory
    LibraryPath = "Vendor/SPIRV-Cross/lib",
    IncludePath = "Vendor/SPIRV-Cross/include",
    
    -- Library names per platform and configuration
    -- SPIRV-Cross has multiple static libraries that need to be linked
    -- The order matters: core must come last as other libs depend on it
    Libraries = {
        windows = {
            Debug = {
                "spirv-cross-glsl-debug.lib",
                "spirv-cross-hlsl-debug.lib",
                "spirv-cross-msl-debug.lib",
                "spirv-cross-cpp-debug.lib",
                "spirv-cross-reflect-debug.lib",
                "spirv-cross-util-debug.lib",
                "spirv-cross-core-debug.lib"
            },
            Release = {
                "spirv-cross-glsl-release.lib",
                "spirv-cross-hlsl-release.lib",
                "spirv-cross-msl-release.lib",
                "spirv-cross-cpp-release.lib",
                "spirv-cross-reflect-release.lib",
                "spirv-cross-util-release.lib",
                "spirv-cross-core-release.lib"
            }
        },
        macosx = {
            Debug = {
                "spirv-cross-glsl",
                "spirv-cross-hlsl",
                "spirv-cross-msl",
                "spirv-cross-cpp",
                "spirv-cross-reflect",
                "spirv-cross-util",
                "spirv-cross-core"
            },
            Release = {
                "spirv-cross-glsl",
                "spirv-cross-hlsl",
                "spirv-cross-msl",
                "spirv-cross-cpp",
                "spirv-cross-reflect",
                "spirv-cross-util",
                "spirv-cross-core"
            }
        },
        linux = {
            Debug = {
                "spirv-cross-glsl",
                "spirv-cross-hlsl",
                "spirv-cross-msl",
                "spirv-cross-cpp",
                "spirv-cross-reflect",
                "spirv-cross-util",
                "spirv-cross-core"
            },
            Release = {
                "spirv-cross-glsl",
                "spirv-cross-hlsl",
                "spirv-cross-msl",
                "spirv-cross-cpp",
                "spirv-cross-reflect",
                "spirv-cross-util",
                "spirv-cross-core"
            }
        }
    }
}

-- msdf-atlas-gen Configuration (static library for MSDF text atlas generation)
Dependencies.msdfAtlasGen = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = false,
    
    -- Paths relative to Engine directory
    LibraryPath = "Vendor/msdf-atlas-gen/lib",
    IncludePath = "Vendor/msdf-atlas-gen",
    
    -- Library names per platform and configuration
    -- msdf-atlas-gen depends on msdfgen-core and msdfgen-ext
    Libraries = {
        windows = {
            Debug = {
                "msdf-atlas-gen-debug.lib",
                "msdfgen-ext-debug.lib",
                "msdfgen-core-debug.lib"
            },
            Release = {
                "msdf-atlas-gen-release.lib",
                "msdfgen-ext-release.lib",
                "msdfgen-core-release.lib"
            }
        },
        macosx = {
            Debug = {
                "msdf-atlas-gen",
                "msdfgen-ext",
                "msdfgen-core"
            },
            Release = {
                "msdf-atlas-gen",
                "msdfgen-ext",
                "msdfgen-core"
            }
        },
        linux = {
            Debug = {
                "msdf-atlas-gen",
                "msdfgen-ext",
                "msdfgen-core"
            },
            Release = {
                "msdf-atlas-gen",
                "msdfgen-ext",
                "msdfgen-core"
            }
        }
    }
}

-- Freetype Configuration (static library, dependency of msdfgen-ext)
Dependencies.Freetype = {
    Version = "latest",
    HeaderOnly = false,
    LibraryPath = "Vendor/msdf-atlas-gen/lib",  -- Copied to same lib directory as msdf-atlas-gen
    IncludePath = "Vendor/freetype/include",
    Libraries = {
        windows = {
            Debug = "freetype-debug.lib",
            Release = "freetype-release.lib"
        },
        macosx = {
            Debug = "freetype",
            Release = "freetype"
        },
        linux = {
            Debug = "freetype",
            Release = "freetype"
        }
    }
}

-- OpenAL Soft Configuration (static library for 3D audio)
Dependencies.OpenALSoft = {
    -- Version string (update when pinning to specific version)
    Version = "latest",
    HeaderOnly = false,
    
    -- Static runtime setting must match how OpenAL Soft was built
    StaticRuntime = true,
    
    -- Paths relative to Engine directory
    LibraryPath = "Vendor/openal-soft/lib",
    IncludePath = "Vendor/openal-soft/include",
    
    -- Library names per platform and configuration
    Libraries = {
        windows = {
            Debug = "OpenAL32-debug.lib",
            Release = "OpenAL32-release.lib"
        },
        macosx = {
            Debug = "openal",
            Release = "openal"
        },
        linux = {
            Debug = "openal",
            Release = "openal"
        }
    }
}

return Dependencies