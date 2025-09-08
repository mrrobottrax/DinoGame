#include "pch.h"

#include "exception.h"

WindowsException::WindowsException() {
  allocate_message(nullptr, GetLastError());
}

WindowsException::WindowsException(HRESULT result) {
  DWORD err = HRESULT_CODE(result);
  allocate_message(nullptr, err);
}

WindowsException::WindowsException(const char *message) {
  allocate_message(message, GetLastError());
}

WindowsException::WindowsException(const char *message, HRESULT result) {
  DWORD err = HRESULT_CODE(result);
  allocate_message(message, err);
}

void WindowsException::allocate_message(const char *message, DWORD err) {
  free(m_message);
  m_message = nullptr;

  // get error string
  LPWSTR wideErr = nullptr;
  DWORD wideErrLen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
                                        FORMAT_MESSAGE_IGNORE_INSERTS |
                                        FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL, err, 0, (LPWSTR)&wideErr, 0, NULL);

  char *mbErr = nullptr;
  if (wideErr != nullptr) {
    // convert to utf8
    int mbErrLen = WideCharToMultiByte(CP_UTF8, 0, wideErr, wideErrLen + 1,
                                       NULL, 0, NULL, NULL);

    mbErr = (char *)malloc(mbErrLen);
    WideCharToMultiByte(CP_UTF8, 0, wideErr, wideErrLen + 1, mbErr, mbErrLen, NULL,
                        NULL);

    LocalFree(wideErr);
  }

  // get length then format
  constexpr const char format[] = "%s:\n%s";

  size_t bufferLen = snprintf(nullptr, 0, format, message, mbErr);
  char *str = (char *)malloc(bufferLen);
  snprintf(str, bufferLen, format, message, mbErr);

  m_message = str;
}