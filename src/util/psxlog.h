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
#include <util/psxutil.h>

namespace Psx {
namespace Log {

enum class MsgType {
    Info,
    Warning,
    Error,
};

void Init(std::ostream& ofile, bool logging_enabled);
void Log(const std::string& label, const std::string& msg, Psx::Log::MsgType t);
void ILog(const std::string& label, const std::string& msg);
void WLog(const std::string& label, const std::string& msg);
void ELog(const std::string& label, const std::string& msg);

}
}

#ifdef PSX_LOGGING
#define PSXLOG_INFO(label, ...) Psx::Log::ILog(label, PSX_FMT(__VA_ARGS__))
#define PSXLOG_WARN(label, ...) Psx::Log::WLog(label, PSX_FMT(__VA_ARGS__))
#define PSXLOG_ERROR(label, ...) Psx::Log::ELog(label, PSX_FMT(__VA_ARGS__))
#else
#define PSXLOG_INFO(label, msg, ...)
#define PSXLOG_WARN(label, msg, ...)
#define PSXLOG_ERROR(label, msg, ...)
#endif

