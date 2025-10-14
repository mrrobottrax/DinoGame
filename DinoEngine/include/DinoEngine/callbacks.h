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
  const char *WindowName = "Dino Engine";
  size_t StaticLevelHeapSize = (1 << 20) * 256; // Default: 256MB
  size_t StaticLevelResourceCapacity = 1024;    // Default: 1024
  size_t StagingBufferCapacity = 1 << 26;       // Default: 64MB
};

DEFINE_CALLBACK(GameInfo, get_game_info);
DEFINE_CALLBACK(void, load_main_menu);