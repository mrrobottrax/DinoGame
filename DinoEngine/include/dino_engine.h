#pragma once

#include "tier0.h"

#include "dino_math.h"

#define DINO_API _declspec(dllimport)
#define GAME_API extern "C" _declspec(dllexport)

#include "DinoEngine/callbacks.h"

#include "DinoEngine/entities.h"

#include "DinoEngine/systems.h"

#include "DinoEngine/ui.h"
