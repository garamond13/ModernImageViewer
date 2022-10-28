#pragma once

#include "targetver.h"
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <shellapi.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <Shlobj.h>
#include <commdlg.h>
#include <array>
#include <vector>
#include <filesystem>

#define OIIO_STATIC_DEFINE
#include <OpenImageIO/imageio.h>

#include <lcms2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Shell32.lib")

#ifdef NDEBUG
#pragma comment(lib, "OpenImageIO.lib")
#pragma comment(lib, "OpenImageIO.lib")
#else
#pragma comment(lib, "OpenImageIO_d.lib")
#pragma comment(lib, "OpenImageIO_Util_d.lib")
#endif

#pragma comment(lib, "OneCoreUAP.lib")
#pragma comment(lib, "Shlwapi.lib")

//insert manifest, enable visual styles for win32 controls
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")