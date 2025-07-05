project "Core"
   kind "WindowedApp" -- TODO: StaticLibrary
   language "C++"
   cppdialect "C++17"
   targetdir "Bin/%{cfg.buildcfg}"
   staticruntime "off"
   location "Core"
   
   -- NuGet 패키지 참조
   nuget { "Microsoft.Direct3D.D3D12:1.616.1" }

   -- PCH
   pchheader "Pch.h"
   pchsource "Core/Pch.cpp" 

   files { "Core/**.h", "Core/**.cpp" }

   includedirs
   {
      "Core"
   }

   targetdir ("../Bin/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Bin/Intermediate/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
        local pkgroot = os.getenv("USERPROFILE") .. "/.nuget/packages/microsoft.direct3d.d3d12/1.616.1/build/native"
        includedirs { pkgroot .. "/include" }
       defines { }

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