#pragma once

#include "game_info.h"

#ifdef DINO_ENGINE
#define DEFINE_CALLBACK(returnType, name, ...)                                 \
  constexpr char k_##name##_name[] = #name;                                    \
  typedef returnType (*##name##_ptr_t)(__VA_ARGS__);
#else
#define DEFINE_CALLBACK(returnType, name, ...)                                 \
  constexpr char k_##name##_name[] = #name;                                    \
  typedef returnType (*##name##_ptr_t)(__VA_ARGS__);                           \
  GAME_API returnType name##(__VA_ARGS__);
#endif

DEFINE_CALLBACK(GameInfo, get_game_info)
DEFINE_CALLBACK(void, load_game_menu)