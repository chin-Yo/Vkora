#include "Logging/Logger.hpp"
#include <iostream>
std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;

void Logger::Init(const std::string& name, LogLevel level,
                  const std::string& filepath, bool async,
                  size_t max_file_size, size_t max_files)
{
    if (async)
    {
        spdlog::init_thread_pool(8192, 1);
    }

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    if (!filepath.empty())
    {
        try
        {
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filepath, max_file_size, max_files);
            sinks.push_back(file_sink);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to initialize file logging:" << e.what() << std::endl;
            if (sinks.empty())
            {
                sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            }
        }
    }

    try
    {
        if (async)
        {
            logger_ = std::make_shared<spdlog::async_logger>(
                name,
                sinks.begin(),
                sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block
            );
        }
        else
        {
            logger_ = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        }


        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
        SetLevel(level);
        logger_->flush_on(spdlog::level::warn);

        spdlog::set_error_handler([](const std::string& msg)
        {
            std::cerr << "spdlog error: " << msg << std::endl;
        });
    }
    catch (const std::exception& e)
    {
        logger_ = spdlog::default_logger();
        std::cerr << "Log initialization failed: " << e.what() << std::endl;
    }
}

void Logger::SetLevel(LogLevel level)
{
    if (logger_)
    {
        logger_->set_level(static_cast<spdlog::level::level_enum>(level));
    }
}
