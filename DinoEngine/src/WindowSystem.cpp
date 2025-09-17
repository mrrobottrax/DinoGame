#include "pch.h"

#include "RenderingSystem.h"
#include "WindowSystem.h"

constexpr wchar_t k_WindowClassName[] = L"Dino Window";
constexpr COLORREF k_BgColor = 0x00181818;

static WNDCLASS s_WndClass;
static HCURSOR s_Cursor;

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

  case WM_SIZE:
    g_RenderingSystem.try_resize(LOWORD(lParam), HIWORD(lParam));
    break;

  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT) {
      SetCursor(s_Cursor);
    } else {
      return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    break;

  default:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

void WindowSystem::init(const char *name, int width, int height,
                        bool resizeable) {
  // create window class
  if (s_WndClass.hInstance == nullptr) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = k_WindowClassName;
    //wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = NULL;

    RegisterClass(&wc);

    s_WndClass = wc;
  }

  // load cursor
  if (s_Cursor == NULL) {
    s_Cursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0,
                                  LR_DEFAULTSIZE | LR_SHARED);
  }

  // create window
  int maxWidth = GetSystemMetrics(SM_CXSCREEN);
  int maxHeight = GetSystemMetrics(SM_CYSCREEN);

  DWORD style =
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

  if (resizeable)
    style |= WS_THICKFRAME;

  DWORD exStyle = WS_EX_NOREDIRECTIONBITMAP;

  RECT rect = {0, 0, (int)width, (int)height};
  if (!AdjustWindowRectEx(&rect, style, FALSE, exStyle)) {
    CRASH_WIN("Failed to adjust rect");
  }

  int w = min(rect.right - rect.left, maxWidth);
  int h = min(rect.bottom - rect.top, maxHeight);

  int x = (maxWidth - w) / 2;
  int y = (maxHeight - h) / 2;

  // convert name to wc
  int wcLen = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
  wchar_t *wcName = (wchar_t *)malloc(wcLen * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, name, -1, wcName, wcLen);

  hWnd = CreateWindowEx(exStyle, k_WindowClassName, wcName, style, x, y, w, h,
                        NULL, NULL, GetModuleHandle(NULL), NULL);

  free(wcName);

  if (hWnd == NULL) {
    CRASH_WIN("Failed to create window");
  }

  DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &k_BgColor,
                        sizeof(k_BgColor));
  DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
  DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference,
                        sizeof(preference));
}

void WindowSystem::stop() {}

void WindowSystem::show_finally() { ShowWindowAsync(hWnd, SW_SHOW); }
