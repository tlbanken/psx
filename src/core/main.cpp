/*
 * psx.cpp
 * 
 * Travis Banken
 * 12/4/2020
 * 
 * Main file for the PSX project.
 */

#include <iostream>
#include <fstream>
#include <memory>

#include "fmt/core.h"

#include "util/psxlog.h"
#include "util/psxutil.h"

int main()
{
    fmt::print("Hello, World!\n");

    // std::ofstream ofile("test.log");
    // psxlog::init(ofile, true);
    psxlog::init(std::cerr, true);
    psxlog::log(__FUNCTION__, "Hello this is a log!", psxlog::MsgType::Info);
    u32 x = 34;
    psxlog::log("TEST", fmt::format("Testing my u32 {}", x), psxlog::MsgType::Info);
    psxlog::log("CPU", "Hello this is another log!", psxlog::MsgType::Error);
    psxlog::log("GPU", "Hello this is another log!", psxlog::MsgType::Warning);
    return 0;
}
