/*
 * psxlog.cpp
 *
 * Travis Banken
 * 12/4/2020
 *
 * Provide basic logging for the psx system.
 */
#include "psxlog.hh"

#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
#include <chrono>

#include "fmt/format.h"
#include "fmt/color.h"
#include "fmt/os.h"
#include "fmt/ostream.h"

static bool s_logOn = false;

/*
 * Allows for the log stream to be either a file or stdout/stderr.
 * This is a hack, not sure how else to do it in c++
 */
static std::ostream& logStream(std::ostream& ofile = std::cerr)
{
    static std::ostream& s_ofile = ofile;
    return s_ofile;
}

// https://stackoverflow.com/questions/16077299/how-to-print-current-time-with-milliseconds-using-c-c11
static std::string getCurrentTimestamp()
{
    // current state
    static long first_ms = 0;

    using std::chrono::system_clock;
    auto currentTime = std::chrono::system_clock::now();
    auto transformed = currentTime.time_since_epoch().count() / 1000000;

    auto millis = transformed;
    if (first_ms == 0) {
        first_ms = millis;
    }

    long ms_diff = millis - first_ms;

    return fmt::format("{:010}", ms_diff);
}

namespace Psx {
namespace Log {

void Init(std::ostream& ofile, bool loggingEnabled)
{
    s_logOn = loggingEnabled;
    logStream(ofile);
}

void Log(const std::string& label, const std::string& msg, Psx::Log::MsgType t)
{
    // don't do anything if no logging
    if (!s_logOn) {
        return;
    }

    auto color = fmt::terminal_color::white;
    char let = '?';

    switch (t) {
    case MsgType::Info:
        color = fmt::terminal_color::blue;
        let = 'I';
        break;
    case MsgType::Warning:
        color = fmt::terminal_color::yellow;
        let = 'W';
        break;
    case MsgType::Error:
        color = fmt::terminal_color::red;
        let = 'E';
        break;
    default:
        // should never get here
        assert(0);
    }

    // format: [<Time in ms>] <MsgType> <LABEL>        <MSG>
    std::ostream& o = logStream();
    o << fmt::format("[{}] ", getCurrentTimestamp());
    // only add color to terminals, not files
    if (&o != &std::cerr && &o != &std::cout) {
        o << fmt::format("{}", let);
    } else {
        o << fmt::format(fg(color), "{}", let);
    }
    o << fmt::format(" {:<15} {}\n", label, msg);
    o.flush();
}

// Logs that omit the type
void ILog(const std::string& label, const std::string& msg)
{
    Log(label, msg, Psx::Log::MsgType::Info);
}

void WLog(const std::string& label, const std::string& msg)
{
    Log(label, msg, Psx::Log::MsgType::Warning);
}

void ELog(const std::string& label, const std::string& msg)
{
    Log(label, msg, Psx::Log::MsgType::Error);
}


}// end ns
}

