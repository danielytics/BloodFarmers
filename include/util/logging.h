#ifndef LOGGING_H
#define LOGGING_H

#include <exception>
#include <spdlog/spdlog.h>

#include <string>

namespace logging {
    void init (const std::string& log_level);
    void term ();
}

#ifdef DEBUG_BUILD
#include "util/clock.h"
class tracing {
public:
    static bool profiling_enabled;

    tracing (const std::string& name)
        : name(name)
        , start_time(Clock::now())
    {
        #ifdef SPDLOG_TRACE_ON
        spdlog::trace("ENTER -- {}", name);
        #endif
    }
    ~tracing () {
        if (profiling_enabled) {
            auto duration = std::chrono::duration_cast<DeltaTime>(Clock::now() - start_time).count() * 1000;
            spdlog::info("PROFILING -- {} = {:.6f} ms", name, duration);
        }
        #ifdef SPDLOG_TRACE_ON
        spdlog::trace("EXIT -- {}", name);
        #endif
    }

private:
    const std::string name;
    const Clock::time_point start_time;
};
#define trace_fn(...) tracing(__FUNCTION__)
#define trace_block(block) tracing(std::string(__FUNCTION__) + std::string("/") + block)
#else
class tracing {
public:
    tracing (const std::string& name) {}
};
#define trace_fn(...)
#define trace_block(block) 
#endif

#define LOG_FILELINE_FMT_ "({}:{}:{}) "

static constexpr const char * const past_last_slash(const char * const str, const char * const last_slash)
{
    return
        *str == '\0' ? last_slash :
        *str == '/'  ? past_last_slash(str + 1, str + 1) :
                       past_last_slash(str + 1, last_slash);
}

static constexpr const char * const past_last_slash(const char * const str) 
{ 
    return past_last_slash(str, str);
}
#define __SHORT_FILE__ ({constexpr const char * const sf__ {past_last_slash(__FILE__)}; sf__;})


#define LOG_(L, fmt, ...) spdlog::L(LOG_FILELINE_FMT_ fmt, __SHORT_FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

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
