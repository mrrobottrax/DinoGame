#pragma once

class DINO_API Entity {
  const char *m_name;

public:
  const char *get_name();
  void set_name(const char *name);
  void set_parent(Entity *parent);
};

DINO_API Entity *entity_spawn(const char *name);