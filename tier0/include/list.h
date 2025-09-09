#pragma once

template <typename T> class List {
  unsigned int m_count;
  unsigned int m_capacity;
  T *m_ptr;

public:
  List(unsigned int capacity = 0) : m_count(0), m_ptr(0), m_capacity(capacity) {
    if (m_capacity > 0) {
      m_ptr = new T[m_capacity];
    }
  }

  List(const List<T> &other) = delete;
  List(List<T> &&other) = delete;

  List<T> &operator=(const List<T> &other) = delete;
  List<T> &operator=(List<T> &&other) = delete;

  ~List() { delete[] m_ptr; }

  T &operator[](unsigned int index) {
    if (index > m_capacity) {
      return *(T *)0;
    }

    return m_ptr[index];
  }

  unsigned int count() { return m_count; }
  unsigned int capacity() { return m_capacity; }

  void add(const T &val) {
    if (m_count + 1 > m_capacity) {
      if (m_capacity == 0) {
        m_capacity = 1;
      }

      reserve(m_capacity * 2);
    }

    m_ptr[m_count++] = val;
  }

  void add(T &&val) {
    if (m_count + 1 > m_capacity) {
      reserve(m_capacity * 2);
    }

    m_ptr[m_count++] = move(val);
  }

  void reserve(unsigned int capacity) {
    T *pOld = m_ptr;
    unsigned int oldCapacity = m_capacity;

    m_capacity = capacity;
    m_ptr = new T[m_capacity];

    for (unsigned int i = 0; i < m_count && i < m_capacity; ++i) {
      m_ptr[i] = move(pOld[i]);
    }

    delete[] pOld;
  }
};