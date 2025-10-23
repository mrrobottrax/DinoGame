#pragma once

struct GameInfo {
  const char *WindowName = "Dino Engine";
  size_t GPUHeapSize = (1 << 20) * 256;      // Default: 256MB
  size_t GPUStagingBufferCapacity = 1 << 26; // Default: 64MB
  uint32_t GPUResourceCapacity = 1024;       // Max number of GPU resources
  uint32_t ShaderCapacity = 256;             // Max number of shaders
};

#define GAME_CALLBACKS_LIST                                                    \
  GAME_CALLBACK(GameInfo, get_game_info)                                       \
  GAME_CALLBACK(void, game_start)

#if DINO_ENGINE
#define GAME_CALLBACK(returnType, name, ...)                                   \
  typedef returnType (*##name##_t)(__VA_ARGS__);                               \
  constexpr char name##_name[] = #name;
#else
#define GAME_CALLBACK(returnType, name, ...)                                   \
  GAME_API returnType name##(__VA_ARGS__);
#endif

GAME_CALLBACKS_LIST

#undef GAME_CALLBACK