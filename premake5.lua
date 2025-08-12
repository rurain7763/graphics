vcpkg_root = os.getenv("VCPKG_ROOT")
vulkan_sdk = os.getenv("VULKAN_SDK")

workspace "graphics"
    architecture "x86_64"
    configurations { "Debug", "Release" }

project "graphics"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
    objdir "%{wks.location}/bin-int/%{cfg.buildcfg}"

    pchheader "pch.h"
    pchsource "src/pch.cpp"

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "./src",
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/include",
        vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/include/Imath",
        vulkan_sdk .. "/Include",
    }

    filter "system:windows"
        links {
            "d3d11.lib",
            "d3dcompiler.lib",
            "Ws2_32.lib",
            "winmm.lib",
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/debug/lib",
            vulkan_sdk .. "/Lib"
        }

        links {
            "spdlogd.lib",
            "assimp-vc143-mtd.lib",
            "OpenEXR-3_3_d.lib",
            "Imath-3_1_d.lib",
            "vulkan-1.lib",
            "fmtd.lib"
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

        libdirs {
            vcpkg_root .. "/installed/%{cfg.architecture:gsub('x86_64','x64')}-%{cfg.system}/lib",
            vulkan_sdk .. "/Lib"
        }

        links {
            "spdlog.lib",
            "assimp-vc143-mt.lib",
            "OpenEXR-3_3.lib",
            "Imath-3_1.lib",
            "vulkan-1.lib",
            "fmt.lib"
        }

    filter "action:vs*"
        buildoptions { "/utf-8" }