/*
 * psx.cpp
 * 
 * Travis Banken
 * 12/4/2020
 * 
 * Main file for the PSX project.
 */

#include "core/config.h"
#include "core/globals.h"

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "util/psxlog.h"
#include "util/psxutil.h"
#include "core/psx.h"

#define MAIN_INFO(msg) PSXLOG_INFO("Main", msg)
#define MAIN_WARN(msg) PSXLOG_WARN("Main", msg)
#define MAIN_ERR(msg) PSXLOG_ERROR("Main", msg)

int main()
{
    // init global emulation state
    g_emu_state.paused = false;
    g_emu_state.step_instr = false;

    // init the logger
    psxlog::Init(std::cerr, true);

    // print project name and version
    std::cout << PSX_FANCYTITLE(PSX_FMT("{} v{}", PROJECT_NAME, PROJECT_VER));

    std::string bios_path;
    bios_path.append(PROJECT_ROOT_PATH);
    bios_path.append("/bios/SCPH1001.BIN");
    try {
        // create PSX object
        Psx psx(bios_path);
        psx.Run();
    } catch (std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
        return 1;
    } catch (std::exception& e) {
        std::cerr << "Error Occured: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown Fatal Failure Occured!" << std::endl;
        return 1;
    }

    return 0;
}


