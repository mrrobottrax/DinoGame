#pragma once

class WindowManager;

class InputManager {
private:
  UINT m_riBufferSize = 0;
  RAWINPUT *m_riBuffer = nullptr;

public:
  ~InputManager() {
    _aligned_free(m_riBuffer);
    m_riBuffer = nullptr;
    m_riBufferSize = 0;
  }

  void register_raw_input(HWND hWnd);
  void process_input();

private:
  void handle_keyboard(RAWKEYBOARD rawKeyboard);
};