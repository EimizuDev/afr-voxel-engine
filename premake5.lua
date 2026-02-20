projectName = _ARGS[1]
projectDir = _ARGS[2]

workspace (projectName)
	location (projectDir)
	configurations { "debug", "release" }
	platforms { "windows", "mac", "linux" }
	architecture "x86_64"
	startproject (projectName)
	staticruntime "Off"
	systemversion "latest"
	defaultplatform "windows"
	
	filter "configurations:debug"
		defines "AFRE_DEBUG"
		runtime "Debug"
		optimize "Off"
		symbols "On"

	filter "configurations:release"
		defines "AFRE_RELEASE"
		runtime "Release"
		optimize "On"
		symbols "Off"

	filter "platforms:windows"
		system "windows"

	filter "platforms:mac"
		system "macosx"

	filter "platforms:linux"
		system "linux"

outputDir = "%{cfg.buildcfg}-%{cfg.platform}"

dirs = {}
dirs["glfw"] = "vendor/glfw"
dirs["vk"] = os.getenv("VULKAN_SDK")
if not dirs["vk"] then
	error("Vulkan's environment path not set!")
end
dirs["vkb"] = "vendor/vk-bootstrap"
dirs["log"] = "vendor/spdlog"
dirs["glm"] = "vendor/glm"
dirs["dense"] = "vendor/unordered_dense"
dirs["entt"] = "vendor/entt"

project (projectName)
	location (projectDir)
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	buildoptions "/utf-8"

	targetdir (projectDir .. "/binaries/" .. outputDir)
	objdir (projectDir .. "/intermediate/" .. outputDir)

	files
	{
		(projectDir .. "/src/**.h"),
		(projectDir .. "/src/**.cpp")
	}

	includedirs
	{
		(projectDir .. "/src"),
		"src",
		"%{dirs.glfw}/include",
		"%{dirs.log}/include",
		"%{dirs.vk}/Include",
		"%{dirs.vkb}/src",
		"%{dirs.glm}",
		"%{dirs.dense}/include",
		"%{dirs.entt}"
	}
	
	links "afr-engine"

	dependson "afr-engine"

	filter "platforms:windows"
		defines "AFRE_WINDOWS"

	filter "platforms:mac"
		defines "AFRE_MAC"

	filter "platforms:linux"
		defines "AFRE_LINUX"

project "afr-engine"
	location (_WORKING_DIR)
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	buildoptions "/utf-8"

	targetdir ("binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("intermediate/" .. outputDir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
		"assets/src/shaders/**.slang"
	}

	includedirs 
	{
		"src",
		"%{dirs.glfw}/include",
		"%{dirs.log}/include",
		"%{dirs.vk}/Include",
		"%{dirs.vkb}/src",
		"%{dirs.glm}",
		"%{dirs.dense}/include",
		"%{dirs.entt}"
	}

	links 
	{
		"glfw",
		"%{dirs.vk}/Lib/vulkan-1.lib",
		"vk-bootstrap"
	}

	dependson "vk-bootstrap"

	filter "platforms:windows"
		defines "AFRE_WINDOWS"

	filter "platforms:mac"
		defines "AFRE_MAC"

	filter "platforms:linux"
		defines "AFRE_LINUX"

project "glfw"
	location "%{dirs.glfw}"
	kind "StaticLib"
	language "C"

	targetdir ("binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("intermediate/" .. outputDir .. "/%{prj.name}")

	files
	{
		"%{dirs.glfw}/include/GLFW/glfw3.h",
		"%{dirs.glfw}/include/GLFW/glfw3native.h",
		"%{dirs.glfw}/src/internal.h",
		"%{dirs.glfw}/src/platform.h",
		"%{dirs.glfw}/src/mappings.h",
		"%{dirs.glfw}/src/context.c",
		"%{dirs.glfw}/src/init.c",
		"%{dirs.glfw}/src/input.c",
		"%{dirs.glfw}/src/monitor.c",
		"%{dirs.glfw}/src/platform.c",
		"%{dirs.glfw}/src/vulkan.c",
		"%{dirs.glfw}/src/window.c",
		"%{dirs.glfw}/src/null_platform.h",
		"%{dirs.glfw}/src/null_joystick.h",
		"%{dirs.glfw}/src/null_init.c",
		"%{dirs.glfw}/src/null_monitor.c",
		"%{dirs.glfw}/src/null_window.c",
		"%{dirs.glfw}/src/null_joystick.c",
		"%{dirs.glfw}/src/egl_context.c",
		"%{dirs.glfw}/src/osmesa_context.c"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	filter "platforms:windows"
		files
		{
			"%{dirs.glfw}/src/win32_init.c",
			"%{dirs.glfw}/src/win32_module.c",
			"%{dirs.glfw}/src/win32_joystick.c",
			"%{dirs.glfw}/src/win32_monitor.c",
			"%{dirs.glfw}/src/win32_time.h",
			"%{dirs.glfw}/src/win32_time.c",
			"%{dirs.glfw}/src/win32_thread.h",
			"%{dirs.glfw}/src/win32_thread.c",
			"%{dirs.glfw}/src/win32_window.c",
			"%{dirs.glfw}/src/win32_platform.h",
			"%{dirs.glfw}/src/win32_joystick.h",
			"%{dirs.glfw}/src/wgl_context.c"
		}

		defines
		{
			"_GLFW_WIN32"
		}

	filter "platforms:mac"
		files
		{
			"%{dirs.glfw}/src/cocoa_time.h",
			"%{dirs.glfw}/src/cocoa_time.c",
			"%{dirs.glfw}/src/posix_thread.h",
			"%{dirs.glfw}/src/posix_module.c",
			"%{dirs.glfw}/src/posix_thread.c",
			"%{dirs.glfw}/src/cocoa_platform.h",
			"%{dirs.glfw}/src/cocoa_joystick.h",
			"%{dirs.glfw}/src/cocoa_init.m",
			"%{dirs.glfw}/src/cocoa_joystick.m",
			"%{dirs.glfw}/src/cocoa_monitor.m",
			"%{dirs.glfw}/src/cocoa_window.m",
			"%{dirs.glfw}/src/nsgl_context.m"
		}

		defines
		{
			"_GLFW_COCOA"
		}

	filter "platforms:linux"
		files
		{
			"%{dirs.glfw}/src/wl_platform.h",
			"%{dirs.glfw}/src/wl_init.c",
			"%{dirs.glfw}/src/wl_monitor.c",
			"%{dirs.glfw}/src/wl_window.c",
			"%{dirs.glfw}/src/posix_time.h",
			"%{dirs.glfw}/src/posix_thread.h",
			"%{dirs.glfw}/src/posix_module.c",
			"%{dirs.glfw}/src/posix_time.c",
			"%{dirs.glfw}/src/posix_thread.c",
			"%{dirs.glfw}/src/linux_joystick.h",
			"%{dirs.glfw}/src/linux_joystick.c",
			"%{dirs.glfw}/src/posix_poll.h",
			"%{dirs.glfw}/src/posix_poll.c"
		}

		defines
		{
			"_GLFW_WAYLAND"
		}

project "vk-bootstrap"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	location "%{dirs.vkb}"

	targetdir ("binaries/" .. outputDir .. "/%{prj.name}")
	objdir ("intermediate/" .. outputDir .. "/%{prj.name}")

	includedirs 
	{
		"%{dirs.vk}/Include",
		"%{dirs.vkb}/src"
	}

	files
	{
		"%{dirs.vkb}/src/*.h",
		"%{dirs.vkb}/src/*.cpp"
	}

	links "%{dirs.vk}/Lib/vulkan-1.lib"