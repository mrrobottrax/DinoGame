#pragma once

#pragma warning(push, 0)

#define D3DCOMPILE_DEBUG 1

#include <DXGIDebug.h>
#include <Windows.h>
#include <d3d12.h>
#include <dbghelp.h>
#include <dwmapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <tlhelp32.h>
#include <wrl.h>

// This is controversial
using namespace Microsoft::WRL;

#pragma warning(pop)