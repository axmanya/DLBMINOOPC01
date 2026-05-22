#include "LogManager.h"
#include <ctime>
#include <iostream>
#include <fstream>

// initialization is needed otherwise error is thrown header is only defition of structure
LogManager *LogManager::instance = nullptr;

/**
 * Returns a string value representing the enum value
 * @param level enum with the log level state
 * @return string with the representing enum value
 */
std::string LogManager::getLogLevelAsString(LogLevel level) {
    switch (level) {
        case LogLevel::DEB:
            return "DEB";
        case LogLevel::INF:
            return "INF";
        case LogLevel::WRN:
            return "WRN";
        case LogLevel::ERR:
            return "ERR";
        case LogLevel::CRT:
            return "CRT";
        default:
            return "";
    }
}

/**
 * Formats the current date and time as console log format
 * @return string with the formatted date and time
 */
std::string LogManager::getFormattedDateTime() {
    char buffer[256];

    // get the current system time
    auto currentTime = std::chrono::system_clock::now();

    // convert the time point to a date time
    std::time_t dateTime = std::chrono::system_clock::to_time_t(currentTime);
    std::tm *now = std::localtime(&dateTime);
    strftime(buffer, sizeof(buffer), "[%Y-%m-%d][%H:%M:%S]", now);
    return std::string(buffer);
}

/**
 * Singleton pattern to ensure there is only one instance of the logger present
 * @return current instance of the LogManager only once created
 */
LogManager *LogManager::getInstance() {
    if (instance == nullptr) {
        instance = new LogManager();
    }
    return instance;
}

/**
 * Set the log level for the log
 * @param level set the new log level
 */
void LogManager::setLogLevel(LogLevel level) {
    logLevel = level;
}

/**
 * Set the path to the log file
 * @param path string that represents the path to the log file
 */
void LogManager::setLogFilePath(const std::string &path) {
    logFilePath = path;
}

/**
 * Set the log file name
 * @param name string value that represents the log file name
 */
void LogManager::setLogFileName(const std::string &name) {
    logFileName = name;
}

/**
 * Defines if the log should be written to a log file, by default, disabled
 * @param enable if on true, the log will be written to a file
 */
void LogManager::setLogToFile(bool enable) {
    logToFile = enable;
}

/**
 * Defines if the log should be written to the console, by default, enabled
 * @param enable if on true, the log will be written to the console
 */
void LogManager::setLogToConsole(bool enable) {
    logToConsole = enable;
}

/**
 * Defines the prefix of the log message
 * @param prefix string value that is used as a prefix for the log message
 */
void LogManager::setLogPrefix(const std::string &prefix) {
    logPrefix = prefix;
}

/**
 * Base log function that is logging if the log level is high enough and bases if the console or file logging is active
 * @param prefix string value that is used as a prefix for the log message
 * @param message string value that is logged
 * @param level log level that is used to determine if the log should be written
 */
void LogManager::Log(const std::string prefix, const std::string &message, LogLevel level) {
    if (level < logLevel)
        return;

    // define the log message string
    std::string logText = getFormattedDateTime() + "[" + getLogLevelAsString(level) + "]" + (
                              !prefix.empty() ? "[" + prefix + "]" : "") + message;

    // if console output enabled use cout
    if (logToConsole)
        std::cout << logText << std::endl;

    // if file output enabled use ofstream to append
    if (logToFile) {
        std::ofstream logFileStream(logFilePath + logFileName, std::ios::app);
        if (logFileStream.is_open()) {
            logFileStream << logText << std::endl;
            logFileStream.close();
        } else {
            std::cerr << "Failed to open log file" << std::endl;
        }
    }
}

/**
 * Log a message without a prefix is calling the base method with the default prefix
 * @param message string value that is logged
 * @param level log level that is used to determine if the log should be written
 */
void LogManager::Log(const std::string &message, LogLevel level) {
    Log(logPrefix, message, level);
}

/**
 * Short for a message of level debug
 * @param message string value that is logged
 */
void LogManager::Debug(const std::string &message) {
    Log(message, LogLevel::DEB);
}

/**
 * Short for a message of level information
 * @param message string value that is logged
 */
void LogManager::Information(const std::string &message) {
    Log(message, LogLevel::INF);
}

/**
 * Short for a message of level warning
 * @param message string value that is logged
 */
void LogManager::Warning(const std::string &message) {
    Log(message, LogLevel::WRN);
}

/**
 * Short for a message of level error
 * @param message string value that is logged
 */
void LogManager::Error(const std::string &message) {
    Log(message, LogLevel::ERR);
}

/**
 * Short for a message of level critical
 * @param message string value that is logged
 */
void LogManager::Critical(const std::string &message) {
    Log(message, LogLevel::CRT);
}

/**
 * Short for a message of level debug with a given prefix
 * @param prefix string value that is used as prefix instead default value
 * @param message string value that is logged
 */
void LogManager::Debug(const std::string &prefix, const std::string &message) {
    Log(prefix, message, LogLevel::DEB);
}

/**
 * Short for a message of level information with a given prefix
 * @param prefix string value that is used as prefix instead default value
 * @param message string value that is logged
 */
void LogManager::Information(const std::string &prefix, const std::string &message) {
    Log(prefix, message, LogLevel::INF);
}

/**
 * Short for a message of level warning with a given prefix
 * @param prefix string value that is used as prefix instead default value
 * @param message string value that is logged
 */
void LogManager::Warning(const std::string &prefix, const std::string &message) {
    Log(prefix, message, LogLevel::WRN);
}

/**
 * Short for a message of level error with a given prefix
 * @param prefix string value that is used as prefix instead default value
 * @param message string value that is logged
 */
void LogManager::Error(const std::string &prefix, const std::string &message) {
    Log(prefix, message, LogLevel::ERR);
}

/**
 * Short for a message of level critical with a given prefix
 * @param prefix string value that is used as prefix instead default value
 * @param message string value that is logged
 */
void LogManager::Critical(const std::string &prefix, const std::string &message) {
    Log(prefix, message, LogLevel::CRT);
}
