-- premake5.lua
workspace "Parang"
   platforms "x64"
   architecture "x86_64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Sandbox"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Parang"
	include "Parang/Build-Core.lua"
group ""

include "Sandbox/Build-Sandbox.lua"