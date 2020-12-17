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

#include "fmt/core.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "glad/glad.h"
#include "glfw/glfw3.h" // Important: include glfw AFTER glad

#include "util/psxlog.h"
#include "util/psxutil.h"
#include "core/psx.h"

#define MAIN_INFO(msg) PSXLOG_INFO("Main", msg)
#define MAIN_WARN(msg) PSXLOG_WARN("Main", msg)
#define MAIN_ERR(msg) PSXLOG_ERROR("Main", msg)

int main()
{
    // init global emulation state
    g_emuState.paused = false;
    g_emuState.stepInstr = false;

    // init the logger
    psxlog::init(std::cerr, true);

    // print project name and version
    std::cout << PSX_FANCYTITLE(PSX_FMT("{} v{}", PROJECT_NAME, PROJECT_VER));

    try {
        // create PSX object
        Psx psx;
        psx.run();
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}


