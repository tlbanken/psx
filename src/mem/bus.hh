/*
 * bus.h
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Header file for the Bus class for the PSX project.
 */

#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Bus {

enum class RWVerbosity {
    Default,
    Quiet,
};

// State Modifiers
void Init();
void Reset();


// Reads
template<class T>
T Read(u32 addr, Bus::RWVerbosity verb = Bus::RWVerbosity::Default);

// Writes
template<class T>
void Write(T data, u32 addr, Bus::RWVerbosity verb = Bus::RWVerbosity::Default);

}
}
