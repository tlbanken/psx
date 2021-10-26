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
    auto cur_sec = std::chrono::system_clock::now();

    if ((cur_sec - last_sec) > std::chrono::duration<double>(1.0)) {
        last_sec = cur_sec;
        return true;
    }

    return false;
}

long long GetDeltaTime()
{
    using namespace std::chrono;
    static auto last = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto ns = duration_cast<nanoseconds>(now - last);
    last = now;
    return ns.count();
}

}// end ns
}
