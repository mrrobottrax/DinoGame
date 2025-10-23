#pragma once

class WindowSystem {
public:
  void start(const char *name, int width = 1280, int height = 720,
            bool resizeable = false);
  void stop();
  void show_finally() const;

  uint32_t get_width() const;
  uint32_t get_height() const;

  HWND get_hWnd() const { return m_hWnd; }

private:
  HWND m_hWnd{};
};

inline WindowSystem g_WindowSystem{};