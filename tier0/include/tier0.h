#pragma once

#define T0_API _declspec(dllimport)

#include "Tier0/base.h"
#include "Tier0/ucrt.h"

#include "Tier0/platform.h"

#include "Tier0/error.h"
#include "Tier0/malloca_janitor.h"
#include "Tier0/todo.h"

#include "Tier0/console.h"
#include "Tier0/xxhash.h"

constexpr size_t k_MaxPathString = 259;