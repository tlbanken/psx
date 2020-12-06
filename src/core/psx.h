/*
 * psx.h
 * 
 * Travis Banken
 * 12/5/2020
 * 
 * Header file for the main PSX class.
 */

#pragma once

#include <memory>

#include "mem/bus.h"

class Psx {
private:
    std::shared_ptr<Bus> m_bus;

public:
    Psx();
    void step();
};
