#pragma once

class WindowSystem {
public:
  void init(const char *name, int width = 1280, int height = 720,
            bool resizeable = false);
  void stop();
  void show_finally();

  uint32_t get_width();
  uint32_t get_height();

  HWND get_hWnd() const { return m_hWnd; }

private:
  HWND m_hWnd;
};

inline WindowSystem g_WindowSystem{};