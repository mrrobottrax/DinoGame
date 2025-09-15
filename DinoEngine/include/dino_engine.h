#pragma once

#include "tier0.h"

#define DINO_API _declspec(dllimport)
#define GAME_API extern "C" _declspec(dllexport)

#include "callbacks.h"
#include "dino_gui.h"
