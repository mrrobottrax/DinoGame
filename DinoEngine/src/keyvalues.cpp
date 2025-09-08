#include "pch.h"

#include "keyvalues.h"

KVObject KVObject::parse(const char *file, unsigned int length) {
  KVObject object{};
  object.m_type = KV_OBJECT;
  object.m_dictVal = KVDict();

  return object;
}

KVObject::KVDict::KVDict() {
  m_capacity = 1;
  m_count = 0;
  m_pEntries = new Entry[m_capacity];
}

KVObject::KVDict::~KVDict() { delete[] m_pEntries; }

KVObject *KVObject::KVDict::get(const char *key) {
  if (m_count + 1 >= m_capacity) {
    resize(m_capacity * 2);
  }

  unsigned int i =
      murmur3_32(key, strnlen_s(key, (size_t)1 << 16), 0x9747b28c) % m_capacity;

  Entry *pEntry = &m_pEntries[i];
  while (pEntry) {
    if (strcmp(key, pEntry->key) == 0) {
      return pEntry->pValue;
    }

    pEntry = pEntry->pNext;
  }

  return nullptr;
}
