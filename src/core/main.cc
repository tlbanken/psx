/*
 * psx.cpp
 * 
 * Travis Banken
 * 12/4/2020
 * 
 * Main file for the PSX project.
 */

#include "core/config.hh"
#include "core/globals.hh"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

// for sleep
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "util/psxlog.hh"
#include "util/psxutil.hh"
#include "core/sys.hh"
#include "cpu/cpu.hh"

#define MAIN_INFO(...) PSXLOG_INFO("Main", __VA_ARGS__)
#define MAIN_WARN(...) PSXLOG_WARN("Main", __VA_ARGS__)
#define MAIN_ERR(...) PSXLOG_ERROR("Main", __VA_ARGS__)

int main()
{
    // init global emulation state
    g_emu_state.paused = false;
    g_emu_state.step_instr = false;

    // init the logger
    Psx::Log::Init(std::cerr, true);

    // print project name and version
#ifdef PSX_DEBUG
    std::cout << PSX_FANCYTITLE(PSX_FMT("{} v{}-debug", PROJECT_NAME, PROJECT_VER));
#else
    std::cout << PSX_FANCYTITLE(PSX_FMT("{} v{}", PROJECT_NAME, PROJECT_VER));
#endif

#ifdef PSX_DEBUG_WAIT_FOR_ATTACH
    volatile int done = 0;
    while (!done) {
        MAIN_INFO("Waiting for debugger to attach...");
        sleep(1);
    }
#endif

    int rc = 0;
    std::string bios_path;
    bios_path.append(PROJECT_ROOT_PATH);
    bios_path.append("/bios/SCPH1001.BIN");
    try {
        // create main System object
        Psx::System psx(bios_path, false);
        psx.Run();
    } catch (std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
        rc = 1;
    } catch (std::invalid_argument& e) {
        std::cerr << "Invalid Arg Exception: " << e.what() << std::endl;
        rc = 1;
    } catch (std::out_of_range& e) {
        std::cerr << "Out of Range Exception: " << e.what() << std::endl;
        rc = 1;
    } catch (std::exception& e) {
        std::cerr << "Error Occured: " << e.what() << std::endl;
        rc = 1;
    } catch (...) {
        std::cerr << "Unknown Fatal Failure Occured!" << std::endl;
        rc = 1;
    }

    MAIN_INFO("PC at exit: 0x{:08x}", Psx::Cpu::GetPC());

    return rc;
}


