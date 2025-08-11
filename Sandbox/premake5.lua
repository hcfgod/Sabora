project "Sandbox"
    location "%{wks.basedir}/Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"

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
    }

    -- Link against Engine and static libraries
    links { "Engine" }

    filter "system:windows"
        links { "setupapi", "winmm", "imm32", "version", "ole32", "oleaut32", "uuid" }
        buildoptions { "/utf-8" }

    filter "system:macosx"

    filter "system:linux"

    filter "configurations:Debug"
        runtime "Debug"

    filter "configurations:Release"
        runtime "Release"


