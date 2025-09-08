#pragma once

#define GAME_API extern "C" _declspec(dllimport)
#define DINO_API extern "C" _declspec(dllexport)

#include "tier0.h"

#include <dwmapi.h>
#include <hidsdi.h>

typedef unsigned __int64 QWORD;