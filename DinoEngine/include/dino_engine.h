#pragma once

#include "tier0.h"

#include "dino_math.h"

#define DINO_API _declspec(dllimport)
#define GAME_API extern "C" _declspec(dllexport)

#include "DinoEngine/IAssetSystem.h"
#include "DinoEngine/ILevelSystem.h"
#include "DinoEngine/IRenderingSystem.h"
#include "DinoEngine/IUISystem.h"
#include "DinoEngine/UI_ColoredPanel.h"
#include "DinoEngine/UI_Grid.h"
#include "DinoEngine/UI_Image.h"
#include "DinoEngine/UI_Panel.h"
#include "DinoEngine/callbacks.h"
