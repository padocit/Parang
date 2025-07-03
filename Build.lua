-- premake5.lua
workspace "Parang"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Sample"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Parang"
	include "Parang/Build-Core.lua"
group ""

include "Sample/Build-Sample.lua"