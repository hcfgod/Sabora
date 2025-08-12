project "Sandbox"
    location "%{wks.basedir}/Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

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
        "../Engine/Vendor/spdlog/include",
        "../Engine/Vendor/doctest",
        "../Engine/Vendor/glm",
        "../Engine/Vendor/json/include",
        "../Engine/Vendor/SDL/include",
    }

    -- Link against Engine (which already contains SDL3)
    links { "Engine" }

    -- SDL3 static linking is handled by Engine project
    defines { "SDL_STATIC" }

    filter "system:windows"
        -- Windows system libraries needed by SDL3
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }

    filter { "system:windows", "configurations:Debug" }
        runtime "Debug"

    filter { "system:windows", "configurations:Release" }
        runtime "Release"

    filter "system:macosx"

    filter "system:linux"

    filter {}