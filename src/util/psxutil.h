/*
 * psxutil.h
 * 
 * Travis Banken
 * 12/4/2020
 * 
 * Utility header file for the psx project.
 */

#pragma once

#include <cstdint>

#include "fmt/core.h"

#include "util/psxlog.h"

// shorter type names
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define PSX_FANCYTITLE(title) fmt::format("┌{0:─^{2}}┐\n│{1: ^{2}}│\n└{0:─^{2}}┘\n", "", title, 20)
