workspace "Rongine"
	architecture "x86_64"
	startproject "Sandbox"

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
includeDir["ImGui"]="Rongine/vendor/imgui"
includeDir["glm"]="Rongine/vendor/glm"
includeDir["stb_image"]="Rongine/vendor/stb_image"
include "Rongine/vendor/GLFW"
include "Rongine/vendor/Glad"
include "Rongine/vendor/imgui"

project "Rongine"
	location "Rongine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" ..outputdir .. "/%{prj.name}")

	pchheader("Rongpch.h")
	pchsource("Rongine/src/Rongpch.cpp")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{prj.name}/src/Rongine/Events",
		"%{prj.name}/src/Rongine/Core",
		"%{includeDir.GLFW}",
		"%{includeDir.Glad}",
		"%{includeDir.ImGui}",
		"%{includeDir.glm}",
		"%{includeDir.stb_image}"
	}

	links
	{
		"GLFW",
		"Glad",
		"opengl32.lib",
		"ImGUi"
	}

	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }

		defines
		{
			"RONG_PLATFORM_WINDOWS",
			"RONG_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "RONG_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "RONG_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "RONG_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	buildoptions "/utf-8"

	debugdir "Sandbox"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" ..outputdir .. "%{prj.name}")

	pchheader("Rongpch.h")
	pchsource("Sandbox/src/Rongpch.cpp")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/assets/**"
	}

	includedirs
	{
		"Rongine/vendor/spdlog/include",
		"Rongine/src",
		"%{includeDir.glm}",
		"Rongine/vendor",
		"%{includeDir.Glad}"
	}

	links
	{
		"Rongine"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"RONG_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "RONG_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "RONG_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "RONG_DIST"
		runtime "Release"
		optimize "on"



