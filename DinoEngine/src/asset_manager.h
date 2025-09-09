#pragma once

enum EAssetStatus : byte {
  ASSET_LOADING,
  ASSET_PARSING,
  ASSET_LOADED,
};

struct Asset {
  const char *path;
  OVERLAPPED overlapped;
  HANDLE hFile;
  EAssetStatus status;

  // larger with malloc
  unsigned int length;
  alignas(8) char data;
};

class AssetManager {
public:
  AssetManager();
  ~AssetManager();

  Asset *precache(const char *path);
  void asset_barrier(Asset *pAsset);
  bool unload(Asset *pAsset);

private:
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