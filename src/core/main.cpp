/*
 * psx.cpp
 * 
 * Travis Banken
 * 2020
 * 
 * Main file for the PSX project.
 */

#include <iostream>
#include "fmt/core.h"

int main()
{
    fmt::print("Hello, World!\n");
    std::cout << fmt::format("Format hex for {} is 0x{:04x}\n", 123, 123);
    return 0;
}
