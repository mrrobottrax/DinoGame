#pragma once

#if DINO_ENGINE
#define DEFINE_CALLBACK(returnType, name, ...)                                 \
  typedef returnType (*##name##_ptr)(__VA_ARGS__);                             \
  constexpr char name##_name[] = #name
#else
#define DEFINE_CALLBACK(returnType, name, ...)                                 \
  GAME_API returnType name##(__VA_ARGS__)
#endif

struct GameInfo {
  const char *windowName;
};

DEFINE_CALLBACK(GameInfo, get_game_info);
DEFINE_CALLBACK(void, load_main_menu);