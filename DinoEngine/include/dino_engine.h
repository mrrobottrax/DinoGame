#pragma once

#include "tier0.h"

#include "dino_gui.h"
#include "dino_math.h"

#define DINO_API _declspec(dllimport)
#define GAME_API extern "C" _declspec(dllexport)

#include "DinoEngine/IAssetSystem.h"
#include "DinoEngine/callbacks.h"
