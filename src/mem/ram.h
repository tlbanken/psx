/*
 * ram.h
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Header for the Ram class in th PSX project.
 */

#pragma once

#include <vector>
#include <memory>

#include "mem/bus.h"
#include "layer/imgui_layer.h"

class Ram final : public AddressSpace, public ImGuiLayer::DbgModule {
private:
    std::vector<u8> m_sysram;


    std::string hexDumpLine(u32 addr);
public:
    Ram();

    // ** Adress Space **
    void Reset();
    // reads
    ASResult Read8(u32 addr);
    ASResult Read16(u32 addr);
    ASResult Read32(u32 addr);
    // writes
    bool Write8(u8 data, u32 addr);
    bool Write16(u16 data, u32 addr);
    bool Write32(u32 data, u32 addr);

    // ** Debug Module **
    std::string GetModuleLabel();
    void OnActive(bool *active);
};
