#include "pch.h"

GAME_API GameInfo get_game_info() {
  return GameInfo{
      .window =
          {
              .name = "Dino Game",
              .width = 1280,
              .height = 720,
          },
  };
}