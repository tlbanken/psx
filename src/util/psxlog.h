/*
 * psxlog.h
 *
 * Travis Banken
 * 12/4/2020
 *
 * Header file for logging.
 */

#pragma once

#include "core/config.h"

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
    void ilog(const std::string& label, const std::string& msg);
    void wlog(const std::string& label, const std::string& msg);
    void elog(const std::string& label, const std::string& msg);
}

#ifdef PSX_LOGGING
#define PSXLOG_INFO(label, msg) psxlog::ilog(label, msg)
#define PSXLOG_WARN(label, msg) psxlog::wlog(label, msg)
#define PSXLOG_ERROR(label, msg) psxlog::elog(label, msg)
#else
#define PSXLOG_INFO(label, msg)
#define PSXLOG_WARN(label, msg)
#define PSXLOG_ERROR(label, msg)
#endif

