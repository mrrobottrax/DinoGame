#pragma once

#include "entity.h"

typedef void (*button_callback_t)();

class DINO_API UITextButton : public Entity {
public:
  void set_text(const char *text);
  void set_on_click(button_callback_t callback);
};

class DINO_API UIMenu : public Entity {
public:
  void open();
  void close();
};
