#include "pch.h"

#include "EntitySystem.h"

DINO_API IEntitySystem *g_IEntitySystem = &g_EntitySystem;

void EntitySystem::start() {}
void EntitySystem::stop() {}