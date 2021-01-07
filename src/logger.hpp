#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <mutex>

// Class to help with thread-safeish logging
class Logger
{
public:
    enum class LogLevel
    {
        Debug = 1,
        Warning,
        Info,
        Error,
        Fatal
    };
    static LogLevel GlobalLogLevel;

    Logger(std::string prefix, LogLevel logLevel)
    {
        this->prefix = prefix;
        this->myLogLevel = logLevel;
    }

    // Logging function - pass any number of args that can be passed to ostream, comma separated
    template <typename T>
    void Log(T out, bool end = true)
    {
        if (GlobalLogLevel > myLogLevel)
        {
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(protector);

        std::cout << "[" << prefix << "]: " << out << " ";
        if (!end)
        {
            std::cout << std::flush;
        }
        else
        {
            std::cout << std::endl;
        }
    }

    template <typename Arg, typename... Args>
    void Log(Arg a, Args... out)
    {
        if (GlobalLogLevel > myLogLevel)
        {
            return;
        }
        std::lock_guard<std::recursive_mutex> lock(protector);

        Log(a, false);
        _Log(out...);

        std::cout << std::endl;
    }

protected:
    // Logger helpers - not meant to be called by user
    template <typename Arg, typename... Args>
    void _Log(Arg a, Args... out)
    {
        std::lock_guard<std::recursive_mutex> lock(protector);

        _Log(a);
        _Log(out...);
    }

    template <typename T>
    void _Log(T arg)
    {
        std::lock_guard<std::recursive_mutex> lock(protector);

        std::cout << arg << " ";
    }

    LogLevel myLogLevel;
    std::string prefix;
    static std::recursive_mutex protector;
};

extern Logger Debug;
extern Logger Info;
extern Logger Warning;
extern Logger Error;
extern Logger Fatal;

#endif