#include "pch.h"

#include "engine.h"
#include "input_manager.h"
#include "window_manager.h"

constexpr wchar_t k_windowClassName[] = L"Dino Window";
constexpr COLORREF k_bgColor = 0x00181818;

static WNDCLASS s_wndClass;
static HCURSOR s_cursor;

static LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam) {
  switch (uMsg) {
  case WM_NCCREATE:
    // store user ptr
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, lParam);
    return DefWindowProc(hWnd, uMsg, wParam, lParam);

  case WM_CLOSE:
    DestroyWindow(hWnd);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT) {
      SetCursor(s_cursor);
    } else {
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    break;

  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

void WindowManager::update_window(const WindowInfo *pWindowInfo) {
  if (hWnd == NULL) {
    create_window(pWindowInfo);
  }

  // set window name
  int wcLen =
      MultiByteToWideChar(CP_UTF8, 0, pWindowInfo->name, -1, NULL, 0);
  wchar_t *wcName = (wchar_t *)malloc(wcLen * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, pWindowInfo->name, -1, wcName, wcLen);

  SetWindowTextW(hWnd, wcName);

  free(wcName);
}

void WindowManager::create_window(const WindowInfo *pWindowInfo) {
  // create window class
  if (s_wndClass.hInstance == nullptr) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = k_windowClassName;
    // wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = NULL;

    RegisterClass(&wc);

    s_wndClass = wc;
  }

  // load cursor
  if (s_cursor == NULL) {
    s_cursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0,
                                  LR_DEFAULTSIZE | LR_SHARED);
  }

  // create window
  int maxWidth = GetSystemMetrics(SM_CXSCREEN);
  int maxHeight = GetSystemMetrics(SM_CYSCREEN);

  DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  DWORD exStyle = WS_EX_NOREDIRECTIONBITMAP;

  RECT rect = {0, 0, (int)pWindowInfo->width,
               (int)pWindowInfo->height};
  if (!AdjustWindowRectEx(&rect, style, FALSE, exStyle)) {
    throw WindowsException("Failed to adjust rect");
  }

  int w = min(rect.right - rect.left, maxWidth);
  int h = min(rect.bottom - rect.top, maxHeight);

  int x = (maxWidth - w) / 2;
  int y = (maxHeight - h) / 2;

  // convert name to wc
  int wcLen =
      MultiByteToWideChar(CP_UTF8, 0, pWindowInfo->name, -1, NULL, 0);
  wchar_t *wcName = (wchar_t *)malloc(wcLen * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, pWindowInfo->name, -1, wcName, wcLen);

  hWnd = CreateWindowEx(exStyle, k_windowClassName, wcName, style, x, y, w, h,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

  free(wcName);

  if (hWnd == NULL) {
    throw WindowsException("Failed to create window");
  }

  DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &k_bgColor,
                        sizeof(k_bgColor));
  DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
  DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference,
                        sizeof(preference));
}
