/*
 * psx.cpp
 * 
 * Travis Banken
 * 12/4/2020
 * 
 * Main file for the PSX project.
 */

#include "core/config.h"

#include <iostream>
#include <fstream>
#include <memory>

#include "fmt/core.h"

#include "util/psxlog.h"
#include "util/psxutil.h"

#define SETUP_INFO(msg) INFO("Setup", msg)
#define SETUP_WARN(msg) WARNING("Setup", msg)
#define SETUP_ERR(msg) ERROR("Setup", msg)

int main()
{
    // init the logger
    psxlog::init(std::cerr, true);

    // print project name and version
    SETUP_INFO(fmt::format("{}: Playstation 1 Emulator!", PROJECT_NAME));
    SETUP_INFO(fmt::format("Version: {}", PROJECT_VER));

    // setup memory
    SETUP_INFO("Initializing system memory...");

    // load BIOS
    SETUP_INFO(fmt::format("Loading BIOS from {}", "<BIOS PATH HERE>"));

    return 0;
}
