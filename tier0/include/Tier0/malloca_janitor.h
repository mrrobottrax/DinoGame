#pragma once

template <typename T> class malloca_janitor {
  T *&m_Alloc;

public:
  malloca_janitor(T *&alloc) : m_Alloc(alloc) {}
  ~malloca_janitor() { _freea(m_Alloc); }
};