#pragma once

class KVObject {
  enum EType {
    KV_INT,
    KV_UINT,
    KV_INT64,
    KV_UINT64,
    KV_FLOAT,
    KV_DOUBLE,
    KV_STRING,
    KV_OBJECT,
    KV_ARRAY,
  };

  EType m_type;

  class KVDict {
    struct Entry {
      const char *key;
      KVObject *pValue;
      Entry *pNext;
    };

    unsigned int m_capacity;
    unsigned int m_count;
    Entry *m_pEntries;

  public:
    KVDict();
    ~KVDict();

    KVObject *get(const char *key);
    void insert(const char *key, KVObject *pValue);

  private:
    void resize(unsigned int capacity);
  };

  union {
    int m_intVal;
    unsigned int m_uintVal;
    long long m_int64Val;
    unsigned long long m_uint64Val;
    float m_floatVal;
    double m_doubleVal;
    struct {
      unsigned int m_stringLen;
      const char *m_stringVal;
    };
    struct {
      unsigned int m_arrLen;
      KVObject *m_arrVal;
    };
    KVDict m_dictVal;
  };

private:
  KVObject() : m_arrVal(0), m_arrLen(0), m_type(KV_ARRAY) {};

public:
  KVObject(const KVObject &other) = delete;
  KVObject(KVObject &&other) noexcept : KVObject() {
    memcpy_s(this, sizeof(KVObject), &other, sizeof(KVObject));
    other.m_type = KV_ARRAY;
    other.m_arrLen = 0;
    other.m_arrVal = nullptr;
  }
  KVObject &operator=(const KVObject &other) = delete;
  KVObject &operator=(KVObject &&other) = delete;

  ~KVObject() {
    if (m_type == KV_ARRAY) {
      delete[] m_arrVal;
    }
  }

  KVObject &operator[](const char *key) {
    if (m_type == KV_OBJECT) {
      return *m_dictVal.get(key);
    }

    return *(KVObject *)0;
  }
  KVObject &operator[](size_t index) {
    if (m_type == KV_ARRAY) {
      if (index >= m_arrLen) {
        return *(KVObject *)0;
      }
      return m_arrVal[index];
    }

    return *(KVObject *)0;
  }

  size_t length() const {
    switch (m_type) {
    case KV_ARRAY:
      return m_arrLen;
    case KV_STRING:
      return m_stringLen;
    }
    return 0;
  }

  long long to_int64() const { return m_int64Val; }
  unsigned long long to_uint64() const { return m_uint64Val; }
  int to_int() const { return m_intVal; }
  unsigned int to_uint() const { return m_uintVal; }
  float to_float() const { return m_floatVal; }
  double to_double() const { return m_doubleVal; }
  const char *cstr() const { return m_stringVal; }

  static KVObject parse(const char *file, unsigned int length);
};
