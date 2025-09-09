#pragma once

class WindowSystem {
private:
  HWND m_hWnd;

public:
  error_t init(const char *name, int width, int height);
  error_t stop();
};