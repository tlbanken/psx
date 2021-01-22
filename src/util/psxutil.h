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
#include <cassert>
#include <string>
#include <iostream>


#include "core/config.h"
#include "fmt/core.h"
#include "fmt/format.h"

#include "util/psxlog.h"

// shorter type names (signed)
typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

// shorter type names (unsigned)
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define PSX_FANCYTITLE(title) fmt::format("┌{0:─^{2}}┐\n│{1: ^{2}}│\n└{0:─^{2}}┘\n", "", title, 20)

// VA_OPT Trick Requires C++20
//#define PSX_FMT(fmtstr, ...) fmt::format(FMT_STRING(fmtstr) __VA_OPT__(,) __VA_ARGS__)
#define PSX_FMT(...) fmt::format(__VA_ARGS__)


#ifdef PSX_DEBUG
#define PSX_ASSERT(cond) assert(cond)
#else
#define PSX_ASSERT(cond)
#endif

