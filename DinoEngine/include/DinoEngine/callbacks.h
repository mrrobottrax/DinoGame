#pragma once

struct GameInfo {
  const char *WindowName = "Dino Engine";
  size_t GPUStagingBufferSize = (1 << 20) * 8; // Default: 8MB
  size_t GPUStaticBufferSize = (1 << 20) * 64; // Default: 64MB
  uint32_t GPUMaxStaticResources = 64;         // Max static CBV, SRV, UAV
  bool CanResizeWindow = false;                // Can drag to resize window
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