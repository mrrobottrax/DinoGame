#pragma once

#pragma warning(push, 0)

#include <DXGIDebug.h>
#include <Windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dbghelp.h>
#include <dwmapi.h>
#include <dxgi1_6.h>
#include <tlhelp32.h>
#include <wrl.h>

#ifndef D3DCOMPILE_DEBUG
#define D3DCOMPILE_DEBUG 1
#endif

// This is controversial
using namespace Microsoft::WRL;

#pragma warning(pop)