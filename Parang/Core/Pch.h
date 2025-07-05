#pragma once

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
	#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#include "TargetVer.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <initguid.h>

// d3d
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d11on12.h>
#include <dwrite.h>
#include <d2d1_3.h>
#include <d3dx12.h>

#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
