/*
 * bios.h
 *
 * Travis Banken
 * 1/9/21
 *
 * Header for the bios loader on the psx.
 */

#pragma once

#include <string>

#include "util/psxutil.h"


namespace Psx {
namespace Bios {

void Init(const std::string& path);
void Init();
void Reset();
void Shutdown();

void LoadFromFile(const std::string& path);
bool IsLoaded();
void OnActive(bool *active);

// reads and writes
template<class T>
T Read(u32 addr);

template<class T>
void Write(T data, u32 addr);

}// end namespace
}

