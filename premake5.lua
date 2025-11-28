workspace "Rongine"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir="%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

includeDir={}
includeDir["GLFW"]="Rongine/vendor/GLFW/include"
includeDir["Glad"]="Rongine/vendor/Glad/include"
include "Rongine/vendor/GLFW"
include "Rongine/vendor/Glad"

project "Rongine"
	location "Rongine"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" ..outputdir .. "/%{prj.name}")

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
		"%{prj.name}/src/Rongine/Core",
		"%{includeDir.GLFW}",
		"%{includeDir.Glad}"
	}

	links
	{
		"GLFW",
		"Glad",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "latest"
		buildoptions { "/utf-8" }

		defines
		{
			"RONG_PLATFORM_WINDOWS",
			"RONG_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		postbuildcommands
		{
			 "{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox"
		}

	filter "configurations:Debug"
		defines "RONG_DEBUG"
		defines "RONG_ENABLE_ASSERTS"
		buildoptions "/MDd"
		symbols "On"

	filter "configurations:Release"
		defines "RONG_RELEASE"
		buildoptions "/MD"
		optimize "On"

	filter "configurations:Dist"
		defines "RONG_DIST"
		buildoptions "/MD"
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
		staticruntime "Off"
		systemversion "10.0"

		defines
		{
			"RONG_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "RONG_DEBUG"
		buildoptions "/MDd"
		symbols "On"

	filter "configurations:Release"
		defines "RONG_RELEASE"
		buildoptions "/MD"
		optimize "On"

	filter "configurations:Dist"
		defines "RONG_DIST"
		buildoptions "/MD"
		optimize "On"



