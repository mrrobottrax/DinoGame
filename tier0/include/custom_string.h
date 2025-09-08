#pragma once

constexpr size_t k_maxStringLength = 1 << 16;

class String {
private:
  struct Container {
    unsigned int refCount;
    char data[1];
  };

  Container *m_ptr;

public:
  String(const char *cstr) {
    size_t strLen = strnlen_s(cstr, k_maxStringLength);
    if (strLen == k_maxStringLength) {
      throw Exception("Max string length reached");
    }

    m_ptr = (Container *)malloc(sizeof(Container) + strLen);
    m_ptr->refCount = 1;
    memcpy_s(&m_ptr->data, strLen + 1, cstr, strLen);
    m_ptr->data[strLen] = '\0';
  }

  String(const String &other) {
    m_ptr = other.m_ptr;
    if (m_ptr)
      ++m_ptr->refCount;
  }

  String(String &&other) {
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
  }

  String &operator=(const String &other) {
    if (&other == this)
      return *this;

    if (m_ptr && --m_ptr->refCount == 0) {
      free(m_ptr);
    }

    m_ptr = other.m_ptr;
    if (m_ptr)
      ++m_ptr->refCount;

    return *this;
  }

  String &operator=(String &&other) {
    if (&other == this)
      return *this;

    if (m_ptr && --m_ptr->refCount == 0) {
      free(m_ptr);
    }

    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;

    return *this;
  }

  ~String() {
    if (m_ptr && --m_ptr->refCount == 0) {
      free(m_ptr);
    }
    m_ptr = nullptr;
  }
};