#include "pch.h"

#include "LevelSystem.h"
#include "UISystem.h"

DINO_API ILevelSystem *g_ILevelSystem = &g_LevelSystem;

void LevelSystem::unload_immediate() {
  ASSERT_ALWAYS(g_UISystem.is_initialized());
  g_UISystem.get_top_panel()->delete_children();
}