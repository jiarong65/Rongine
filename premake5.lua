workspace "Rongine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir="%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Rongine"
	location "Rongine"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" ..outputdir .. "%{prj.name}")

	pchheader("Rongpch.h")
	pchsource("Rongine/src/Rongpch.cpp")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/src/Rongine/Events",
		"%{prj.name}/src/Rongine/Core"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"
		buildoptions { "/utf-8" }

		defines
		{
			"RONG_PLATFORM_WINDOWS",
			"RONG_BUILD_DLL"
		}

		postbuildcommands
		{
			 "{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox"
		}

	filter "configurations:Debug"
		defines "RONG_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "RONG_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "RONG_DIST"
		optimize "On"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	buildoptions "/utf-8"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" ..outputdir .. "%{prj.name}")

	pchheader("Rongpch.h")
	pchsource("Sandbox/src/Rongpch.cpp")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"Rongine/vendor/spdlog/include",
		"Rongine/src"
	}

	links
	{
		"Rongine"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "10.0"

		defines
		{
			"RONG_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "RONG_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "RONG_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "RONG_DIST"
		optimize "On"



