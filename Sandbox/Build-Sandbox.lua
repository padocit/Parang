project "Sandbox"
   kind "WindowedApp"
   language "C++"
   cppdialect "C++17"
   targetdir "Bin/%{cfg.buildcfg}"
   staticruntime "off"
   location "Sandbox"

   files { "src/**.h", "src/**.cpp" }

   includedirs
   {
      "src",

	  -- Include Core
      "../Parang",
	  "../Parang/Core"
   }

   links
   {
      "Core"
   }

   targetdir ("../Bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Bin/Intermediate/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"