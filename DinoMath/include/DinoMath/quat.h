#pragma once

typedef struct {
  float WXYZ[4];
} quat_t;

inline quat_t quat_identity() {
  quat_t q{};

  q.WXYZ[0] = 1;
  q.WXYZ[1] = 0;
  q.WXYZ[2] = 0;
  q.WXYZ[3] = 0;

  return q;
}