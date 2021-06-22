/*
 * ram.h
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Header for the Ram class in th PSX project.
 */

#pragma once

#include "util/psxutil.hh"

namespace Psx {
namespace Ram {

void Init();
void Reset();

template<class T>
T Read(u32 addr);

template<class T>
void Write(T data, u32 addr);

void OnActive(bool *active);

}// end namespace
}

