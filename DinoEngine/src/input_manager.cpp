#include "pch.h"

#include "engine.h"
#include "input_manager.h"
#include "window_manager.h"

void InputManager::register_raw_input(HWND hWnd) {
  RAWINPUTDEVICE devices[] = {
      {
          .usUsagePage = HID_USAGE_PAGE_GENERIC,
          .usUsage = HID_USAGE_GENERIC_KEYBOARD,
          .dwFlags = RIDEV_INPUTSINK,
          .hwndTarget = hWnd,
      },
      {
          .usUsagePage = HID_USAGE_PAGE_GENERIC,
          .usUsage = HID_USAGE_GENERIC_MOUSE,
          .dwFlags = RIDEV_INPUTSINK,
          .hwndTarget = hWnd,
      },
  };

  if (RegisterRawInputDevices(devices, _countof(devices), sizeof(devices[0])) ==
      FALSE) {
    throw WindowsException("Failed to register raw input");
  }
}

void InputManager::process_input() {
  // get required size
  UINT requiredSize = 0;
  GetRawInputBuffer(NULL, &requiredSize, sizeof(RAWINPUTHEADER));

  if (requiredSize == 0)
    return;

  USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
  if (!IsWow64Process2(GetCurrentProcess(), &processMachine, NULL))
    return;
  if (processMachine != IMAGE_FILE_MACHINE_UNKNOWN) {
    requiredSize *= 8;
  }

  if (requiredSize > m_riBufferSize) {
    if (m_riBufferSize == 0)
      m_riBufferSize = 4096;

    while (requiredSize > m_riBufferSize) {
      m_riBufferSize *= 2;
    }

    console_log("Resizing raw input buffer to %u bytes", m_riBufferSize);

    _aligned_free(m_riBuffer);
    m_riBuffer = (RAWINPUT *)_aligned_malloc(m_riBufferSize, 8);
    if (!m_riBuffer) {
      console_log_error("Failed to allocate input buffer");
      return;
    }
  }

  // get input
  UINT bufferSize = m_riBufferSize;
  UINT count =
      GetRawInputBuffer(m_riBuffer, &bufferSize, sizeof(RAWINPUTHEADER));
  if (count == UINT_MAX) {
    console_log_error("Error with raw input");
    return;
  }

  RAWINPUT *current = m_riBuffer;
  for (UINT i = 0; i < count; ++i) {
    switch (current->header.dwType) {
    case RIM_TYPEKEYBOARD:
      handle_keyboard(current->data.keyboard);
      break;
    }

    current = NEXTRAWINPUTBLOCK(current);
  }
}

void InputManager::handle_keyboard(RAWKEYBOARD rawKeyboard) {
  if (rawKeyboard.VKey == 'R' && (rawKeyboard.Flags & RI_KEY_BREAK) == 0) {
    g_engine->toggle_reload();
  }
}
