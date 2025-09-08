#pragma once

template <typename T> class SharedPtr {
  struct InternalStruct {
    unsigned int count;
    T value;
  };

private:
  InternalStruct *m_ptr;

  SharedPtr(InternalStruct *p) {
    m_ptr = p;
    if (m_ptr)
      ++m_ptr->count;
  }

public:
  SharedPtr(const SharedPtr<T> &other) : SharedPtr<T>(other.m_ptr) {}
  SharedPtr(SharedPtr<T> &&other) {
    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;
  }

  SharedPtr<T> &operator=(const SharedPtr<T> &other) {
    if (this == &other)
      return *this;

    if (m_ptr && --m_ptr->count == 0) {
      delete m_ptr;
    }

    m_ptr = other.m_ptr;
    if (m_ptr)
      ++m_ptr->count;

    return *this;
  }

  SharedPtr<T> &operator=(SharedPtr<T> &&other) {
    if (this == &other)
      return *this;

    if (m_ptr && --m_ptr->count == 0) {
      delete m_ptr;
    }

    m_ptr = other.m_ptr;
    other.m_ptr = nullptr;

    return *this;
  }

  ~SharedPtr() {
    if (m_ptr && --m_ptr->count == 0) {
      delete m_ptr;
    }
  }

  T *operator->() { return &m_ptr->value; }
  T &operator*() { return m_ptr->value; }

  template <typename... Args> static SharedPtr<T> make(Args &&...args) {
    InternalStruct *p = new InternalStruct{0, T(forward<Args>(args)...)};
    return SharedPtr(p);
  }
};