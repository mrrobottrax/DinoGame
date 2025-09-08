#include "pch.h"

#include "asset_manager.h"

constexpr int k_maxKicks = 32;
constexpr int k_maxRehashes = 64;

AssetManager::AssetManager() {
  m_nEntries = 0;
  m_capacityPerTable = 1024;

  alloc_entries(&m_pEntries1, m_capacityPerTable);
  alloc_entries(&m_pEntries2, m_capacityPerTable);
}

AssetManager::~AssetManager() {
  free_entries(m_pEntries1, m_capacityPerTable);
  free_entries(m_pEntries2, m_capacityPerTable);
  m_pEntries1 = nullptr;
  m_pEntries2 = nullptr;

  m_nEntries = 0;
  m_capacityPerTable = 0;
}

static void completion_routine(_In_ DWORD dwErrorCode,
                               _In_ DWORD dwNumberOfBytesTransfered,
                               _Inout_ LPOVERLAPPED lpOverlapped) {
  Asset *pAsset = CONTAINING_RECORD(lpOverlapped, Asset, overlapped);

  pAsset->status = ASSET_LOADED;

  if (!CloseHandle(pAsset->hFile)) {
    throw WindowsException("Failed to close file handle");
  }

  if (!SetEvent(pAsset->overlapped.hEvent)) {
    throw WindowsException("Failed to set event");
  }
}

Asset *AssetManager::precache(const char *path) {
  Asset *pAsset;
  if (get_asset(path, &pAsset)) {
    pAsset->context = m_pDefaultContext;
    return pAsset;
  }

  console_log("Opening file: %s", path);

  constexpr wchar_t k_prefix[] = L"assets\\";
  constexpr int k_prefixLen = sizeof(k_prefix) / sizeof(k_prefix[0]) - 1;

  int wcLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
  wcLen += k_prefixLen;
  wchar_t *wcPath = (wchar_t *)malloc(wcLen * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, path, -1, wcPath + k_prefixLen,
                      wcLen - k_prefixLen);

  memcpy_s(wcPath, k_prefixLen * sizeof(wchar_t), k_prefix,
           k_prefixLen * sizeof(wchar_t));

  HANDLE hFile =
      CreateFileW(wcPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (hFile == NULL) {
    free(wcPath);
    console_log_error("Failed to open file: %s", path);
    throw WindowsException("Failed to open file");
  }

  LARGE_INTEGER liFileSize;
  if (!GetFileSizeEx(hFile, &liFileSize)) {
    free(wcPath);
    console_log_error("Failed to get file size: %s", path);
    throw WindowsException("Failed to get file size");
  }

  DWORD fileSize = (DWORD)liFileSize.QuadPart;

  void *buffer = malloc(sizeof(Asset) + fileSize - 1);
  if (buffer == nullptr) {
    free(wcPath);
    console_log_error("Failed to allocate asset buffer: %s", path);
    throw Exception("Failed to allocate asset buffer");
  }

  pAsset = new (buffer) Asset{};
  pAsset->status = ASSET_LOADING;
  pAsset->hFile = hFile;
  pAsset->context = m_pDefaultContext;
  pAsset->length = fileSize;
  pAsset->overlapped.hEvent =
      CreateEventExW(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

  free(wcPath);

  if (pAsset->overlapped.hEvent == NULL) {
    throw WindowsException("Failed to create event");
  }

  insert_asset(path, pAsset);

  if (!ReadFileEx(hFile, &pAsset->data, fileSize, &pAsset->overlapped,
                  completion_routine)) {
    CloseHandle(hFile);
    console_log_error("Failed to read file: %s", path);
    throw WindowsException("Failed to read file");
  }

  return pAsset;
}

void AssetManager::asset_barrier(Asset *pAsset) {
  if (WaitForSingleObjectEx(pAsset->overlapped.hEvent, INFINITE, TRUE) ==
      WAIT_FAILED) {
    throw WindowsException("Failed to wait for object");
  }
}

void AssetManager::context_barrier(const AssetContext *pContext) {
  Entry *pTables[] = {m_pEntries1, m_pEntries2};
  for (int j = 0; j < _countof(pTables); ++j) {
    for (unsigned int i = 0; i < m_capacityPerTable; ++i) {
      Entry &entry = pTables[j][i];
      if (entry.pAsset != nullptr && entry.pAsset->context == pContext) {
        asset_barrier(entry.pAsset);
      }
    }
  }
}

void AssetManager::unload_context(const AssetContext *pContext) {
  Entry *pTables[] = {m_pEntries1, m_pEntries2};
  for (int j = 0; j < _countof(pTables); ++j) {
    for (unsigned int i = 0; i < m_capacityPerTable; ++i) {
      Entry &entry = pTables[j][i];
      if (entry.pAsset != nullptr && entry.pAsset->context == pContext) {
        remove_asset(entry.path);
      }
    }
  }
}

bool AssetManager::unload(Asset *pAsset) { return remove_asset(pAsset->path); }

void AssetManager::alloc_entries(Entry **ppEntries, unsigned int capacity) {
  *ppEntries = (Entry *)malloc(capacity * sizeof(Entry));

  if (*ppEntries == nullptr) {
    throw Exception("Failed to allocate entries");
  }

  for (unsigned int i = 0; i < capacity; ++i) {
    new (&(*ppEntries)[i]) Entry{};
  }
}

void AssetManager::free_entries(Entry *pEntries, unsigned int capacity) {
  for (unsigned int i = 0; i < capacity; ++i) {
    Entry &bucket = pEntries[i];

    free(bucket.pAsset);
    free(bucket.path);

    bucket.pAsset = nullptr;
    bucket.path = nullptr;
  }

  free(pEntries);
}

void AssetManager::resize(unsigned int capacity) {
  unsigned int oldCapacity = m_capacityPerTable;
  Entry *pOld1 = m_pEntries1;
  Entry *pOld2 = m_pEntries2;

  // resize hash map
  m_capacityPerTable *= 2;
  alloc_entries(&m_pEntries1, m_capacityPerTable);
  alloc_entries(&m_pEntries2, m_capacityPerTable);

  // re-insert all old elements
  m_nEntries = 0;
  for (unsigned int i = 0; i < oldCapacity; ++i) {
    Entry &entry = pOld1[i];
    insert_asset(entry.path, entry.pAsset);
  }

  for (unsigned int i = 0; i < oldCapacity; ++i) {
    Entry &entry = pOld2[i];
    insert_asset(entry.path, entry.pAsset);
  }

  // delete old array
  free_entries(pOld1, oldCapacity);
  free_entries(pOld2, oldCapacity);
}

static unsigned int hash1(const char *str) {
  return murmur3_32(str, strnlen_s(str, MAX_PATH), 0x9747b28c);
}

static unsigned int hash2(const char *str) {
  return murmur3_32(str, strnlen_s(str, MAX_PATH), 0x5bd1e995);
}

void AssetManager::insert_asset(const char *path, Asset *pAsset) {
  // all tables %50 full
  if (m_nEntries + 1 >= m_capacityPerTable) {
    resize(m_capacityPerTable * 2);
  }

  size_t pathLen = strnlen_s(path, MAX_PATH);
  char *newPath = (char *)malloc(pathLen + 1);
  memcpy(newPath, path, pathLen + 1);
  newPath[pathLen] = '\0';

  Entry newEntry{
      .path = newPath,
      .pAsset = pAsset,
  };

  pAsset->path = newPath;

  for (int c = 0; c < k_maxRehashes; ++c) {
    for (int i = 0; i < k_maxKicks; ++i) {
      unsigned int h1 = hash1(newEntry.path) % m_capacityPerTable;
      if (m_pEntries1[h1].path == nullptr) {
        m_pEntries1[h1] = newEntry;
        ++m_nEntries;
        return;
      }

      // evict and swap
      Entry temp = m_pEntries1[h1];
      m_pEntries1[h1] = newEntry;
      newEntry = temp;

      unsigned int h2 = hash2(newEntry.path) % m_capacityPerTable;
      if (m_pEntries2[h2].path == nullptr) {
        m_pEntries2[h2] = newEntry;
        ++m_nEntries;
        return;
      }

      // evict again
      temp = m_pEntries2[h2];
      m_pEntries2[h2] = newEntry;
      newEntry = temp;
    }

    resize(m_capacityPerTable * 2);
  }

  free(newEntry.path);
  free(newEntry.pAsset);

  console_log_error("Failed to insert: %s", path);
  throw Exception("Failed to insert into table");
}

bool AssetManager::get_asset(const char *path, Asset **ppAsset) {
  unsigned int h1 = hash1(path) % m_capacityPerTable;

  if (m_pEntries1[h1].path != nullptr) {
    if (strcmp(path, m_pEntries1[h1].path) == 0) {
      if (ppAsset)
        *ppAsset = m_pEntries1[h1].pAsset;
      return true;
    }
  }

  unsigned int h2 = hash2(path) % m_capacityPerTable;

  if (m_pEntries2[h2].path != nullptr) {
    if (strcmp(path, m_pEntries2[h2].path) == 0) {
      if (ppAsset)
        *ppAsset = m_pEntries2[h2].pAsset;
      return true;
    }
  }

  return false;
}

bool AssetManager::remove_asset(const char *path) {
  unsigned int h1 = hash1(path) % m_capacityPerTable;

  if (m_pEntries1[h1].path != nullptr) {
    if (strcmp(path, m_pEntries1[h1].path) == 0) {
      free(m_pEntries1[h1].path);
      free(m_pEntries1[h1].pAsset);
      m_pEntries1[h1].path = nullptr;
      m_pEntries1[h1].pAsset = nullptr;
      return true;
    }
  }

  unsigned int h2 = hash2(path) % m_capacityPerTable;

  if (m_pEntries2[h2].path != nullptr) {
    if (strcmp(path, m_pEntries2[h2].path) == 0) {
      free(m_pEntries2[h2].path);
      free(m_pEntries2[h2].pAsset);
      m_pEntries2[h2].path = nullptr;
      m_pEntries2[h2].pAsset = nullptr;
      return true;
    }
  }

  return false;
}
