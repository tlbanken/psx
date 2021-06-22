/*
 * psxutil.cpp
 *
 * Travis Banken
 * 3/5/2021
 *
 * Collection of useful utility functions to be used in the project.
 */

#include "psxutil.hh"

#include <chrono>

namespace Psx {
namespace Util {

bool OneSecPassed()
{
    static auto last_sec = std::chrono::system_clock::now();
    // using std::chrono::system_clock;
    auto cur_sec = std::chrono::system_clock::now();

    if ((cur_sec - last_sec) > std::chrono::duration<double>(1.0)) {
        last_sec = cur_sec;
        return true;
    }

    return false;
}

}// end ns
}
