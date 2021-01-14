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
#include "core/sys.h"
#include "util/psxlog.h"
#include "util/psxutil.h"

#include "psxtest_mem.h"
#include "psxtest_asm.h"
#include "psxtest_cpu.h"

#define TMAIN_INFO(msg) PSXLOG_INFO("Test-Main", msg)
#define TMAIN_WARN(msg) PSXLOG_WARN("Test-Main", msg)
#define TMAIN_ERROR(msg) PSXLOG_ERROR("Test-Main", msg)

int main()
{
    std::cout << PSX_FANCYTITLE(PSX_FMT("{}-Tests v{}", PROJECT_NAME, PROJECT_VER));
    Psx::Log::Init(std::cout, true);
    Psx::System psx("", true);

    TMAIN_INFO("Starting Memory Tests");
    // call memory tests
    Psx::Test::MemTests();

    // call asm/dasm tests
    TMAIN_INFO("Starting Asm/Dasm Tests");
    Psx::Test::AsmDasmTests();

    // call cpu tests
    TMAIN_INFO("Starting CPU Tests");
    Psx::Test::CpuTests();

    return 0;
}
