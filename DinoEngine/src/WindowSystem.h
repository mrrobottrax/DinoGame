#pragma once

class WindowSystem {
public:
  HWND hWnd;

public:
  void init(const char *name, int width = 1280, int height = 720,
            bool resizeable = false);
  void stop();
  void show_finally();
};

inline WindowSystem g_WindowSystem{};