#pragma once

struct GameInfo;
class Engine;

class WindowManager {
public:
  HWND hWnd = NULL;

  void update_window(const WindowInfo *pWindowInfo);

private:
  void create_window(const WindowInfo *pWindowInfo);
};