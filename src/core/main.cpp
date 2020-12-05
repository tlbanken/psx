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
#include "core/psx.h"

#define MAIN_INFO(msg) psxlog::ilog("Main", msg)
#define MAIN_WARN(msg) psxlog::wlog("Main", msg)
#define MAIN_ERR(msg) psxlog::elog("Main", msg)

int main()
{
    // init the logger
    psxlog::init(std::cerr, true);

    // print project name and version
    MAIN_INFO(fmt::format("{}: Playstation 1 Emulator!", PROJECT_NAME));
    MAIN_INFO(fmt::format("Version: {}", PROJECT_VER));

    Psx psx;

    return 0;
}
