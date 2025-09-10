#pragma once

class WindowSystem {
private:
  HWND m_hWnd;

public:
  void init(const char *name, int width, int height);
  void stop();
};