#pragma once

class KVObject {};

static class KeyValues {
public:
  SharedPtr<KVObject> parse(const char *file);
};