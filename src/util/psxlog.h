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

    void Init(std::ostream& ofile, bool logging_enabled);
    void Log(const std::string& label, const std::string& msg, psxlog::MsgType t);
    void ILog(const std::string& label, const std::string& msg);
    void WLog(const std::string& label, const std::string& msg);
    void ELog(const std::string& label, const std::string& msg);
}

#ifdef PSX_LOGGING
#define PSXLOG_INFO(label, msg) psxlog::ILog(label, msg)
#define PSXLOG_WARN(label, msg) psxlog::WLog(label, msg)
#define PSXLOG_ERROR(label, msg) psxlog::ELog(label, msg)
#else
#define PSXLOG_INFO(label, msg)
#define PSXLOG_WARN(label, msg)
#define PSXLOG_ERROR(label, msg)
#endif

