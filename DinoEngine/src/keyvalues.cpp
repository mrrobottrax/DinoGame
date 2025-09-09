#include "pch.h"

#include "keyvalues.h"

constexpr size_t k_maxKeyLen = 1 << 16;
constexpr size_t k_maxObjectDepth = 1 << 5;

static bool is_whitespace(const char &c) {
  switch (c) {
  case ' ':
    return true;
  case '\t':
    return true;
  case '\r':
    return true;
  case '\n':
    return true;
  }

  return false;
}

static bool is_digit(const char &c) { return c >= '0' && c <= '9'; }

KVObject KVObject::parse(const char *file, unsigned int length) {
  KVObject object{};
  object.m_type = KV_OBJECT;
  object.m_pDictVal = new KVDict();

  unsigned int stackDepth = 0;
  KVObject *pObjStack[k_maxObjectDepth];

  if (stackDepth >= k_maxObjectDepth)
    throw Exception("Max KV object depth exceeded");
  pObjStack[stackDepth] = &object;

  enum EContext {
    CTX_EXPECTING_KEY,
    CTX_READING_KEY,
    CTX_EXPECTING_VALUE,
    CTX_READING_VALUE,
    CTX_READING_COMMENT,
  };

  EContext ctx = CTX_EXPECTING_KEY;
  bool inQuote = false;

  const char *keyStart = nullptr;
  unsigned int keyLength = 0;
  const char *valueStart = nullptr;
  unsigned int valueLength = 0;
  for (unsigned int i = 0; i < length; ++i) {
    const char &c = file[i];

    switch (ctx) {

    case CTX_EXPECTING_KEY: {
      if (is_whitespace(c))
        continue;

      if (!inQuote && c == '}') {
        ctx = CTX_EXPECTING_KEY;
        if (stackDepth == 0)
          throw Exception("Object depth mismatch");
        --stackDepth;
        continue;
      } else if (c == '"') {
        inQuote = true;
      } else {
        inQuote = false;
      }

      keyStart = &c;

      ctx = CTX_READING_KEY;
      continue;
    }

    case CTX_READING_KEY: {
      bool endQuote = inQuote && c == '"';
      bool whiteSpaceNotInQuote = !inQuote && is_whitespace(c);
      if (endQuote || whiteSpaceNotInQuote) {
        inQuote = false;
        keyLength = (int)(&c - keyStart);
        ctx = CTX_EXPECTING_VALUE;

        if (endQuote) {
          keyStart += 1;
          keyLength -= 1;
        }

        continue;
      }

      continue;
    }

    case CTX_EXPECTING_VALUE: {
      if (is_whitespace(c))
        continue;

      if (c == ']') {
        if (pObjStack[stackDepth]->m_type != KV_LIST)
          throw Exception("Malformed list");

        ctx = CTX_EXPECTING_KEY;
        continue;
      }

      if (stackDepth >= k_maxObjectDepth)
        throw Exception("Max KV object depth exceeded");

      KVObject *pNew = new KVObject();

      if (c == '{') {
        pNew->m_type = KV_OBJECT;
        pNew->m_pDictVal = new KVDict();
      } else if (c == '[') {
        pNew->m_type = KV_LIST;
        pNew->m_pListVal = new List<KVObject *>();
      } else
        pNew->m_type = KV_STRING;

      if (pObjStack[stackDepth]->m_type == KV_OBJECT) {
        pObjStack[stackDepth]->m_pDictVal->insert(keyStart, keyLength, pNew);
      } else if (pObjStack[stackDepth]->m_type == KV_LIST) {
        pObjStack[stackDepth]->m_pListVal->add(pNew);
      } else {
        delete pNew;
        throw Exception("Malformed object stack");
      }

      pObjStack[++stackDepth] = pNew;

      valueStart = &c;

      if (c == '"') {
        inQuote = true;
      } else {
        inQuote = false;
      }

      if (c == '{')
        ctx = CTX_EXPECTING_KEY;
      else if (c == '[')
        ctx = CTX_EXPECTING_VALUE;
      else
        ctx = CTX_READING_VALUE;
      continue;
    }

    case CTX_READING_VALUE: {
      bool endQuote = inQuote && c == '"';
      bool whiteSpaceNotInQuote = !inQuote && is_whitespace(c);
      if (endQuote || whiteSpaceNotInQuote) {
        inQuote = false;
        valueLength = (int)(&c - valueStart);

        if (endQuote) {
          pObjStack[stackDepth]->m_type = KV_STRING;
          pObjStack[stackDepth]->m_stringVal = valueStart + 1;
          pObjStack[stackDepth]->m_stringLen = valueLength - 1;
        }
        // TODO: detect other types
        else {
          pObjStack[stackDepth]->m_type = KV_STRING;
          pObjStack[stackDepth]->m_stringVal = valueStart;
          pObjStack[stackDepth]->m_stringLen = valueLength;
        }

        --stackDepth;

        if (pObjStack[stackDepth]->m_type == KV_LIST) {
          ctx = CTX_EXPECTING_VALUE;
        } else {
          ctx = CTX_EXPECTING_KEY;
        }

        continue;
      }

      continue;
    }
    }
  }

  return object;
}

KVObject::KVDict::KVDict() {
  m_capacity = 16;
  m_count = 0;
  m_pEntries = new Entry[m_capacity]{};
}

KVObject::KVDict::~KVDict() { delete[] m_pEntries; }

void KVObject::KVDict::resize(unsigned int capacity) {
  unsigned int oldCapacity = m_capacity;
  Entry *pOld = m_pEntries;

  m_count = 0;
  m_capacity = capacity;
  m_pEntries = new Entry[m_capacity]{};

  for (unsigned int i = 0; i < oldCapacity; ++i) {
    Entry *pEntry = &pOld[i];
    while (pEntry) {
      if (pEntry->key) {
        insert(pEntry->key, (unsigned int)strnlen_s(pEntry->key, k_maxKeyLen),
               pEntry->pValue);
      }
      pEntry = pEntry->pNext;
    }
  }

  delete[] pOld;
}

KVObject *KVObject::KVDict::get(const char *key) {
  unsigned int i =
      murmur3_32(key, (unsigned int)strnlen_s(key, k_maxKeyLen), 0x9747b28c) %
      m_capacity;

  Entry *pEntry = &m_pEntries[i];
  while (pEntry) {
    if (strncmp(key, pEntry->key, pEntry->keyLength) == 0) {
      return pEntry->pValue;
    }

    pEntry = pEntry->pNext;
  }

  return nullptr;
}

void KVObject::KVDict::insert(const char *key, unsigned int keyLength,
                              KVObject *pValue) {
  if (m_count + 1 >= m_capacity) {
    resize(m_capacity * 2);
  }

  unsigned int i = murmur3_32(key, keyLength, 0x9747b28c) % m_capacity;

  Entry *pEntry = &m_pEntries[i];

  while (pEntry) {
    if (pEntry->key == nullptr) {
      break;
    }

    if (strncmp(pEntry->key, key, keyLength) == 0) {
      return;
    }

    if (pEntry->pNext == nullptr) {
      pEntry->pNext = new Entry{};
    }

    pEntry = pEntry->pNext;
  }

  pEntry->key = key;
  pEntry->keyLength = keyLength;
  pEntry->pValue = pValue;

  ++m_count;
}
