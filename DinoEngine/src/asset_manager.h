#pragma once

enum EAssetStatus : byte {
  ASSET_LOADING,
  ASSET_PARSING,
  ASSET_LOADED,
};

class AssetContext {
private:
  char *m_name;

public:
  AssetContext(const char *name) {
    size_t len = strnlen_s(name, 512);
    m_name = (char *)malloc(len + 1);
    memcpy_s(m_name, len + 1, name, len + 1);
  }

  ~AssetContext() {
    free(m_name);
    m_name = nullptr;
  }
};

struct Asset {
  const AssetContext *context;
  OVERLAPPED overlapped;
  HANDLE hFile;
  EAssetStatus status;

  // larger with malloc
  alignas(8) unsigned char data;
};

class AssetManager {
public:
  AssetManager();
  ~AssetManager();

  Asset *precache(const char *path);
  void asset_barrier(Asset *pAsset);
  void unload_assets(const AssetContext *pContext);
  void set_default_context(const AssetContext *pContext) {
    m_pDefaultContext = pContext;
  }

  const AssetContext gameContext{"game"};

private:
  const AssetContext *m_pDefaultContext;

  struct Entry {
    char *path;
    Asset *pAsset;
  };

  unsigned int m_nEntries;
  unsigned int m_capacityPerTable;
  Entry *m_pEntries1;
  Entry *m_pEntries2;

  void alloc_entries(Entry **ppEntries, unsigned int capacity);
  void free_entries(Entry *pEntries, unsigned int capacity);
  void resize(unsigned int capacity);
  void insert_asset(const char *path, Asset *pAsset);
  bool remove_asset(const char *path);
  bool get_asset(const char *path, Asset **ppAsset);
};