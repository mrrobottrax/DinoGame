#pragma once

#include <Windows.h>
#include <dwmapi.h>

#include "ucrt.h"

#include "error.h"

#include "console.h"
#include "hash.h"

error_t t0_init();
error_t t0_stop();