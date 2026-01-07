-- =============================================================
-- [配置] OpenCASCADE 相对路径
-- 指向项目内部的 vendor 目录，确保项目可移植
-- =============================================================
local OCCT_DIR = "Rongine/vendor/OCCT"

-- 需要链接的 OCCT 核心库列表
local OCCT_LIBS = {
	"TKernel",
	"TKMath",
	"TKG3d",
	"TKBRep",
	"TKPrim",
	"TKMesh",
	"TKTopAlgo",
	"TKSTEP",
	"TKIGES",
	"TKShHealing",
	"TKXSBase",    -- 包含 XSControl_Reader
	"TKSTEPBase",  -- STEP 的基础定义
	"TKSTEPAttr",  -- STEP 属性支持
    "TKSTEP209",   -- 额外的 STEP 协议支持
    "TKCAF"        -- OCAF 框架支持 
}
-- =============================================================

workspace "Rongine"
	architecture "x86_64"
	startproject "Rongine-Editor" -- 默认启动编辑器

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- 定义所有依赖的头文件路径
includeDir = {}
includeDir["GLFW"] = "Rongine/vendor/GLFW/include"
includeDir["Glad"] = "Rongine/vendor/Glad/include"
includeDir["ImGui"] = "Rongine/vendor/imgui"
includeDir["glm"] = "Rongine/vendor/glm"
includeDir["entt"] = "Rongine/vendor/entt/include"
includeDir["stb_image"] = "Rongine/vendor/stb_image"
-- [新增] OCCT 头文件路径 (使用 %{wks.location} 确保基于工作区根目录)
includeDir["OCCT"] = "%{wks.location}/" .. OCCT_DIR .. "/inc"
includeDir["ImGuizmo"]="Rongine/vendor/ImGuizmo"

include "Rongine/vendor/GLFW"
include "Rongine/vendor/Glad"
include "Rongine/vendor/imgui"

-- =============================================================
-- Project: Rongine (Core Engine Library)
-- =============================================================
project "Rongine"
	location "Rongine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader("Rongpch.h")
	pchsource("Rongine/src/Rongpch.cpp")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
        "%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	filter "files:**/ImGuizmo.cpp"
        flags { "NoPCH" }
    filter {} -- 重置过滤器，防止影响后面的配置

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
		"%{includeDir.stb_image}",
		"%{includeDir.entt}",
		"%{includeDir.OCCT}", -- [新增] 核心引擎需要包含 OCCT 头文件以编写 CADMesher
		"%{includeDir.ImGuizmo}"
	}

	links
	{
		"GLFW",
		"Glad",
		"opengl32.lib",
		"ImGui"
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

-- =============================================================
-- Project: Sandbox (Test App)
-- =============================================================
project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	buildoptions "/utf-8"

	debugdir "Sandbox"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

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
		"%{includeDir.Glad}",
		"%{includeDir.OCCT}" -- 以防 Sandbox 也需要用到几何类型
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


-- =============================================================
-- Project: Rongine-Editor (Main Application)
-- =============================================================
project "Rongine-Editor"
	location "Rongine-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	buildoptions "/utf-8"

	debugdir "%{prj.name}"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader("Rongpch.h")
	pchsource("Rongine-Editor/src/Rongpch.cpp")

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
		"%{includeDir.Glad}",
		"%{includeDir.entt}",
		"%{includeDir.OCCT}", -- 必须包含，因为 EditorLayer 会调用 OCCT API
		"%{includeDir.ImGuizmo}"
	}

	-- [新增] 添加 OCCT 的库目录
	libdirs
	{
		"%{wks.location}/" .. OCCT_DIR .. "/win64/vc14/lib"
	}

	links
	{
		"Rongine",
		OCCT_LIBS -- [新增] 自动链接 OCCT 库
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"RONG_PLATFORM_WINDOWS"
		}
		
		-- [新增] 构建后自动复制 DLL
		-- 这行命令会自动把 vendor/OCCT/win64/vc14/bin 下的所有 DLL 复制到 exe 旁边
		postbuildcommands
		{
			"{COPY} \"%{wks.location}/" .. OCCT_DIR .. "/win64/vc14/bin/*.dll\" \"%{cfg.targetdir}\""
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