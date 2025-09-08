#pragma once

constexpr int k_maxExceptionLen = 4096;

class Exception {
protected:
  char *m_message;

public:
  Exception() : m_message(nullptr) {}

  Exception(const char *message) : m_message(nullptr) {
    if (message == nullptr)
      return;

    size_t len = strnlen_s(message, k_maxExceptionLen);
    m_message = (char *)malloc(len + 1);
    strncpy_s(m_message, len + 1, message, len);
  }

  ~Exception() {
    free(m_message);
    m_message = nullptr;
  }

  const char *message() { return m_message; }
};

class WindowsException : public Exception {
private:
  void allocate_message(const char *message, DWORD result);

public:
  WindowsException();
  WindowsException(HRESULT result);
  WindowsException(const char *message);
  WindowsException(const char *message, HRESULT result);
};

static inline void win_assert(HRESULT result) {
  if (!SUCCEEDED(result)) {
    throw WindowsException("Assert failed");
  }
}

static inline void win_assert(HRESULT result, const char *message) {
  if (!SUCCEEDED(result)) {
    throw WindowsException(message);
  }
}