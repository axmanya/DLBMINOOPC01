#pragma once
#include <string>

enum class LogLevel {
    DEB,
    INF,
    WRN,
    ERR,
    CRT
};

class LogManager {
private:
    static LogManager *instance;
    LogLevel logLevel = LogLevel::INF;
    std::string logFilePath;
    std::string logFileName;
    std::string logPrefix;
    bool logToFile = false;
    bool logToConsole = true;

public:
    std::string getLogLevelAsString(LogLevel level);

    std::string getFormattedDateTime();

    void operator=(const LogManager &) = delete;

    static LogManager *getInstance();

    void setLogLevel(LogLevel level);

    void setLogFilePath(const std::string &path);

    void setLogFileName(const std::string &name);

    void setLogToFile(bool enable);

    void setLogToConsole(bool enable);

    void setLogPrefix(const std::string &prefix);

    void Log(std::string prefix, const std::string &message, LogLevel level);

    void Log(const std::string &message, LogLevel level);

    void Debug(const std::string &message);

    void Information(const std::string &message);

    void Warning(const std::string &message);

    void Error(const std::string &message);

    void Critical(const std::string &message);

    void Debug(const std::string &prefix, const std::string &message);

    void Information(const std::string &prefix, const std::string &message);

    void Warning(const std::string &prefix, const std::string &message);

    void Error(const std::string &prefix, const std::string &message);

    void Critical(const std::string &prefix, const std::string &message);
};
