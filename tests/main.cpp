/*
 * main.cpp (tests)
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Entry point for unit tests for the PSX project.
 */

#include <iostream>
#include <fstream>

#include "fmt/format.h"

#include "core/config.h"
#include "util/psxlog.h"
#include "util/psxutil.h"

#include "psxtest_mem.h"

#define TMAIN_INFO(msg) PSXLOG_INFO("Test-Main", msg)
#define TMAIN_WARN(msg) PSXLOG_WARN("Test-Main", msg)
#define TMAIN_ERROR(msg) PSXLOG_ERROR("Test-Main", msg)

static std::ofstream testout;

int main()
{
    psxlog::init(std::cerr, true);

    TMAIN_INFO("Starting Memory Tests");
    // call memory tests
    psxtest::memTests();
    return 0;
}
