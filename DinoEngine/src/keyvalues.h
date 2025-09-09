#pragma once

class KVObject {
  enum EType {
    KV_INT,
    KV_UINT,
    KV_DOUBLE,
    KV_STRING,
    KV_OBJECT,
    KV_LIST,
  };

  EType m_type;

  class KVDict {
    struct Entry {
      char *key;
      KVObject *pValue;
      Entry *pNext;

      ~Entry() {
        delete pNext;
        free(key);
      }
    };

    unsigned int m_capacity;
    unsigned int m_count;
    Entry *m_pEntries;

  public:
    KVDict();
    ~KVDict();

    KVObject *get(const char *key);
    void insert(char *key, KVObject *pValue);

  private:
    void resize(unsigned int capacity);
  };

  union {
    long long m_int64Val;
    unsigned long long m_uint64Val;
    double m_doubleVal;
    struct {
      unsigned int m_stringLen;
      const char *m_stringVal;
    };
    List<KVObject *> *m_pListVal;
    KVDict *m_pDictVal;
  };

private:
  KVObject() : m_int64Val(0), m_type(KV_LIST) {};

public:
  KVObject(const KVObject &other) = delete;
  KVObject(KVObject &&other) noexcept : KVObject() {
    memcpy_s(this, sizeof(KVObject), &other, sizeof(KVObject));
    other.m_type = KV_LIST;
    other.m_pListVal = nullptr;
  }
  KVObject &operator=(const KVObject &other) = delete;
  KVObject &operator=(KVObject &&other) = delete;

  ~KVObject() {
    if (m_type == KV_LIST) {
      delete m_pListVal;
    } else if (m_type == KV_OBJECT) {
      delete m_pDictVal;
    }
  }

  KVObject *operator[](const char *key) {
    if (m_type == KV_OBJECT) {
      return m_pDictVal->get(key);
    }

    return nullptr;
  }
  KVObject *operator[](unsigned int index) {
    if (m_type == KV_LIST) {
      return (*m_pListVal)[index];
    }

    return nullptr;
  }

  size_t length() const {
    switch (m_type) {
    case KV_LIST:
      return m_pListVal->count();
    case KV_STRING:
      return m_stringLen;
    }
    return 0;
  }

  long long to_int64() const {
    if (m_type != KV_INT)
      return 0;
    return m_int64Val;
  }
  unsigned long long to_uint64() const {
    if (m_type != KV_UINT)
      return 0;
    return m_uint64Val;
  }
  int to_int() const {
    if (m_type != KV_INT && m_type != KV_UINT)
      return 0;
    return (int)m_int64Val;
  }
  unsigned int to_uint() const {
    if (m_type != KV_INT && m_type != KV_UINT)
      return 0;
    return (unsigned int)m_uint64Val;
  }
  float to_float() const {
    if (m_type != KV_DOUBLE)
      return 0;
    return (float)m_doubleVal;
  }
  double to_double() const {
    if (m_type != KV_DOUBLE)
      return 0;
    return m_doubleVal;
  }
  const char *cstr() const {
    if (m_type != KV_STRING)
      return 0;
    return m_stringVal;
  }

  static KVObject parse(const char *file, unsigned int length);
};
