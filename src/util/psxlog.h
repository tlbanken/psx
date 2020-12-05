/*
 * psxlog.h
 *
 * Travis Banken
 * 12/4/2020
 *
 * Header file for logging.
 */

#pragma once

#include <fstream>
#include <memory>

namespace psxlog {
    enum class MsgType {
        Info,
        Warning,
        Error,
    };

    void init(std::ostream& ofile, bool logging_enabled);
    void log(const std::string& label, const std::string& msg, psxlog::MsgType t);
}

