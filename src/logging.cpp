
#include "logging.h"
// #include "util/Profiling.h"

// #ifdef DEBUG_BUILD
// bool Profile::profiling_enabled;
// #endif

std::shared_ptr<spdlog::logger> logging::logger;

void logging::init () {
    // Async logging in release mode, sync logging in debug mode
    // In a debug build, we want to make sure everything gets logged before a crash
    // In a release build, we would like things to be logged, but performance is more important
#ifndef DEBUG_BUILD
    size_t q_size = 8192; //queue size must be power of 2
    spdlog::set_async_mode(q_size);
#endif
    std::string log_level {"info"};
    // bool profiling = false;
    //  using namespace Config;
    //  auto parser = make_parser(
    //     map("telemetry",
    //         scalar("logging", log_level),
    //         optional(scalar("profiling", profiling))
    //     )
    // );
    // parser(config_node);
    std::map<std::string,spdlog::level::level_enum> log_levels{
        {"trace", spdlog::level::trace},
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"No", spdlog::level::off},
    };
    // TODO: Error checking for invalid values of log_level
    spdlog::level::level_enum level = log_levels[log_level];
    spdlog::set_level(level);
    logging::logger = spdlog::stdout_logger_mt("console", true /*use color*/);
    spdlog::set_pattern("%l [%D %H/%M/%S:%f] %v");
    if (level != spdlog::level::off) {
        info("Logging with level '{}'", log_level);
#ifdef DEBUG_BUILD
        // Set global profiling on or off
        // Profile::profiling_enabled = profiling;
#endif
    }
}

void logging::term () {
    spdlog::drop_all();
}
