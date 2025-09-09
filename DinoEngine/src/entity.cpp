#include "pch.h"

#include "entity.h"

const char *Entity::get_name() { return m_name; }

void Entity::set_name(const char *name) { m_name = name; }

void Entity::set_parent(Entity *pEntity) {
  console_log("Settings parent to %s", pEntity->m_name);
}

Entity *entity_spawn(const char *name) {
  console_log("Spawning %s", name);
  return nullptr;
}