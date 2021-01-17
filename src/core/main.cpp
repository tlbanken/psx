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
#include "core/sys.h"

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
    std::cout << PSX_FANCYTITLE(PSX_FMT("{} v{}", PROJECT_NAME, PROJECT_VER));

    std::string bios_path;
    bios_path.append(PROJECT_ROOT_PATH);
    bios_path.append("/bios/SCPH1001.BIN");
    try {
        // create main System object
        Psx::System psx(bios_path, false);
        psx.Run();
    } catch (std::runtime_error& re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
        return 1;
    } catch (std::invalid_argument& e) {
        std::cerr << "Invalid Arg Exception: " << e.what() << std::endl;
        return 1;
    } catch (std::out_of_range& e) {
        std::cerr << "Out of Range Exception: " << e.what() << std::endl;
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


