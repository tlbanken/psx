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

#include "fmt/core.h"
#include "fmt/format.h"

#include "core/config.hh"
#include "util/psxlog.hh"

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

// define shorter unsigned int (for msvc)
typedef unsigned int uint;

#define PSX_FANCYTITLE(title) fmt::format("┌{0:─^{2}}┐\n│{1: ^{2}}│\n└{0:─^{2}}┘\n", "", title, 20)

// VA_OPT Trick Requires C++20
//#define PSX_FMT(fmtstr, ...) fmt::format(FMT_STRING(fmtstr) __VA_OPT__(,) __VA_ARGS__)
#define PSX_FMT(...) fmt::format(__VA_ARGS__)


#ifdef PSX_DEBUG
// #define PSX_ASSERT(cond) assert(cond)
#define PSX_ASSERT(cond) if (!(cond)) throw std::runtime_error(PSX_FMT("{}:{} Assertion [{}] failed!", __FILE__, __LINE__, #cond));
#else
#define PSX_ASSERT(cond)
#endif

namespace Psx {
namespace Util {

/*
 * Set bits at the given index, of a given size, to given val.
 */
inline void SetBits(u32& word, uint index, uint size, u32 val)
{
    // turn off bits first, then set
    word &= ~(~(0xffff'ffff << size) << index);
    word |= (val << index);
}

/*
 * Get bits at the given index, of a given size.
 */
inline u32 GetBits(u32 word, uint index, uint size)
{
    return (word >> index) & ~(0xffff'ffff << size);
}

/*
 * Return true if one sec has passed since the last call
 */
bool OneSecPassed();

}// end ns
}
