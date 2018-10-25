#ifndef LOGGING_H
#define LOGGING_H

#include <exception>
#include <spdlog/spdlog.h>

namespace logging {
    void init ();
    void term ();

    extern std::shared_ptr<spdlog::logger> logger;
}

#define LOG_FILELINE_FMT "({}:{}:{}) "

#define LOG_(L, fmt, ...) logging::logger->L(LOG_FILELINE_FMT fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#ifdef SPDLOG_TRACE_ON
#define trace(...) LOG_(trace, __VA_ARGS__)
#else
#define trace(...)
#endif

#ifdef SPDLOG_DEBUG_ON
#define debug(...) LOG_(debug, __VA_ARGS__)
#else
#define debug(...)
#endif

#define info(...) LOG_(info, __VA_ARGS__)
#define warn(...) LOG_(warn, __VA_ARGS__)
#define error(...) LOG_(error, __VA_ARGS__)

#define fatal(...) {error(__VA_ARGS__); throw std::runtime_error("Unrecoverable error");}

#endif // LOGGING_H
