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
  size_t GPUHeapSize = (1 << 20) * 256;      // Default: 256MB
  size_t GPUStagingBufferCapacity = 1 << 26; // Default: 64MB
  uint32_t GPUResourceCapacity = 1024;       // Max number of GPU resources
  uint32_t ShaderCapacity = 256;             // Max number of shaders
};

DEFINE_CALLBACK(GameInfo, get_game_info);
DEFINE_CALLBACK(void, load_main_menu);

#undef DEFINE_CALLBACK