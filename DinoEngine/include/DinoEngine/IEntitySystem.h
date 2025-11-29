#pragma once

class IEntitySystem {
public:
  template <typename T> T *create();
};

DINO_API extern IEntitySystem *g_IEntitySystem;

template <typename T> inline T *IEntitySystem::create() {
  T *ent = new T();
  ent->spawn();
  return ent;
}

template <typename T> inline T *entity_create() {
  return g_IEntitySystem->create<T>();
}