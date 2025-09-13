#pragma once

class WindowSystem {
private:
  HWND m_hWnd;

public:
  void init(const char *name, int width = 1280, int height = 720, bool resizeable = false);
  void stop();
};

inline WindowSystem g_WindowSystem;