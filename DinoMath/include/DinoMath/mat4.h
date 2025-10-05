#pragma once

typedef struct {
  float m_Data[4][4];
} mat4_t;

inline mat4_t mat4_create(float x, float y, float z, float w, float h) {
  return mat4_t{{
      {w, 0, 0, 0},
      {0, h, 0, 0},
      {0, 0, 1, 0},
      {x, y, z, 1},
  }};
}