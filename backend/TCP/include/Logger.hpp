#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>


enum class LogLevel {
    TRACE,      // Detailed and used for deep debugging internal functions or variables.
    DEBUG,      // General Debugging info, object transitions, containers information
    INFO,       // High level flow of the program
    WARNING,    // Unexpected behavor occurred but program continues
    ERROR,      // Significant problem occurred
    CRITICAL    // Severe error causing program failure
};

class Logger {
    private:
        inline static LogLevel priority;
        std::string filename;
        std::ofstream file;

        inline static std::mutex log_mtx;

        
        Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        ~Logger() {closeFile();}

        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }


        template<typename... Args>
        void log(int line, const char* sourceFile, LogLevel level, const char* msg, Args... args) {
            if (level < priority) return; 
            char buffer[1024];
            std::lock_guard<std::mutex> lock(log_mtx);
            
            if constexpr(sizeof...(args) == 0) {
                std::snprintf(buffer, sizeof(buffer), "%s", msg);
            } else {
                std::snprintf(buffer, sizeof(buffer), msg, args...);
            }
            
            std::ostringstream oss;
            oss << timestamp() << " [" << levelToStr(level) << "] (" 
                << sourceFile << ":" << line << ") " << buffer << std::endl;

            if (file.is_open()) {
                file << oss.str();
                file.flush();
            }
        }

        template<typename... Args>
        void log(LogLevel level, const char* msg, Args... args) {
            if (level < priority) return; 
            char buffer[1024];
            std::lock_guard<std::mutex> lock(log_mtx);
           
            if constexpr (sizeof...(args) == 0) {
                std::snprintf(buffer, sizeof(buffer), "%s", msg);
            } else {
                std::snprintf(buffer, sizeof(buffer), msg, args...);
            }

            std::ostringstream oss;
            oss << timestamp() << " [" << levelToStr(level) << "] " 
                << buffer << std::endl;

            if (file.is_open()) {
                file << oss.str();
                file.flush();
            }
        }

        void openFile() {
            closeFile();
            file.open(filename, std::ios::app);
            if (!file) throw std::runtime_error("Failed to open log file: " + filename);
        }

        void closeFile() {
            if (file.is_open()) file.close();
        }
        
        static std::string levelToStr(LogLevel level) {
            switch(level) {
                case LogLevel::TRACE: return "TRACE";
                case LogLevel::DEBUG: return "DEBUG";
                case LogLevel::INFO: return "INFO";
                case LogLevel::WARNING: return "WARNING";
                case LogLevel::ERROR: return "ERROR";
                case LogLevel::CRITICAL: return "CRITICAL";
                default: return "UNKNOWN";
            }
        }



        static std::string timestamp() {
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_r(&t, &tm); 

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        static std::string generateLogFileName(const std::string& prefix = "app") {
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_r(&t,  &tm);

            std::ostringstream oss;

            oss << "logs/" << prefix << "_" << std::put_time(&tm, "%y%m%d_%H%M%S") << ".log";
            return oss.str();
        }

    public:
        static void setPriority(LogLevel newPriority) {priority = newPriority;}
        
        static void enableFileOutput(const std::string& newFileName = "") {
            Logger& logger = getInstance();
            logger.filename = (newFileName == "") ? generateLogFileName() : newFileName;
            logger.openFile();
        }

        // Logging w/ line+file info
        template<typename... Args>
        static void trace(int line, const char* sourceFile, const char* msg, Args... args) {
            getInstance().log(line, sourceFile, LogLevel::TRACE, msg, args...);
        }

        template<typename... Args>
        static void debug(int line, const char* sourceFile, const char* msg, Args... args) {
            getInstance().log(line, sourceFile, LogLevel::DEBUG, msg, args...);
        }

        template<typename... Args>
        static void info(int line, const char* sourceFile, const char* msg, Args... args) {
            getInstance().log(line, sourceFile, LogLevel::INFO, msg, args...);

        }
        
        template<typename... Args>
        static void warning(int line, const char* sourceFile, const char* msg, Args... args) {
            getInstance().log(line, sourceFile, LogLevel::WARNING, msg, args...);
        }

        template<typename... Args>
        static void error(int line, const char* sourceFile, const char* msg, Args... args) {
            getInstance().log(line, sourceFile, LogLevel::ERROR, msg, args...);
        }

        template<typename... Args>
        static void critical(int line, const char* sourceFile, const char* msg, Args... args) {
            getInstance().log(line, sourceFile, LogLevel::CRITICAL, msg, args...);
        }


        // Logging w/o line+file info
        template<typename... Args>
        static void trace(const char* msg, Args... args) {
            getInstance().log(LogLevel::TRACE, msg, args...);
        }

        template<typename... Args>
        static void debug(const char* msg, Args... args) {
            getInstance().log(LogLevel::DEBUG, msg, args...);
        }

        template<typename... Args>
        static void info(const char* msg, Args... args) {
            getInstance().log(LogLevel::INFO, msg, args...);
        }
    
        template<typename... Args>
        static void warning(const char* msg, Args... args) {
            getInstance().log(LogLevel::WARNING, msg, args...);
        }

        template<typename... Args>
        static void error(const char* msg, Args... args) {
            getInstance().log(LogLevel::ERROR, msg, args...);
        }

        template<typename... Args>
        static void critical(const char* msg, Args... args) {
            getInstance().log(LogLevel::CRITICAL, msg, args...);
        }

};

#define TRACE_SRC(message, ...) Logger::trace(__LINE__, __FILE__, message, ##__VA_ARGS__)
#define INFO_SRC(message, ...) Logger::info(__LINE__, __FILE__, message, ##__VA_ARGS__)
#define DEBUG_SRC(message, ...) Logger::debug(__LINE__, __FILE__, message, ##__VA_ARGS__)
#define WARNING_SRC(message, ...) Logger::warning(__LINE__, __FILE__, message, ##__VA_ARGS__)
#define ERROR_SRC(message, ...) Logger::error(__LINE__, __FILE__, message, ##__VA_ARGS__)
#define CRITICAL_SRC(message, ...) Logger::critical(__LINE__, __FILE__, message, ##__VA_ARGS__)

#define TRACE(message, ...) Logger::trace(message, ##__VA_ARGS__)
#define INFO(message, ...) Logger::info(message, ##__VA_ARGS__)
#define DEBUG(message, ...) Logger::debug(message, ##__VA_ARGS__)
#define WARNING(message, ...) Logger::warning(message, ##__VA_ARGS__)
#define ERROR(message, ...) Logger::error(message, ##__VA_ARGS__)
#define CRITICAL(message, ...) Logger::critical(message, ##__VA_ARGS__)


#define RETURN_CRITICAL(message, ...) do {CRITICAL_SRC(message, ##__VA_ARGS__); return false; } while(0)
#define RETURN_ERROR(message, ...) do {ERROR_SRC(message, ##__VA_ARGS__); return false; } while(0)
#define RETURN_WARNING(message, ...) do {WARNING_SRC(message, ##__VA_ARGS__); return false; } while(0)

#endif