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

#include "util/psxlog.h"

// shorter type names
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#ifdef LOGGING
#define INFO(label, msg) psxlog::log(label, msg, psxlog::MsgType::Info);
#define WARNING(label, msg) psxlog::log(label, msg, psxlog::MsgType::Warning);
#define ERROR(label, msg) psxlog::log(label, msg, psxlog::MsgType::Error);
#else
#define INFO(label, msg)
#define WARNING(label, msg)
#define ERROR(label, msg)
#endif
