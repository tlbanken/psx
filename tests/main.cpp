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
#include "psxtest_asm.h"
#include "psxtest_cpu.h"

#define TMAIN_INFO(msg) PSXLOG_INFO("Test-Main", msg)
#define TMAIN_WARN(msg) PSXLOG_WARN("Test-Main", msg)
#define TMAIN_ERROR(msg) PSXLOG_ERROR("Test-Main", msg)

static std::ofstream testout;

int main()
{
    psxlog::Init(std::cerr, true);

    std::cout << PSX_FANCYTITLE(PSX_FMT("{}-Tests v{}", PROJECT_NAME, PROJECT_VER));
    TMAIN_INFO("Starting Memory Tests");
    // call memory tests
    psxtest::MemTests();

    // call asm/dasm tests
    TMAIN_INFO("Starting Asm/Dasm Tests");
    psxtest::AsmDasmTests();

    // call cpu tests
    TMAIN_INFO("Starting CPU Tests");
    psxtest::CpuTests();

    return 0;
}
