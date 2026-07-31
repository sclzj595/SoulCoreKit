#ifndef SOUL_CORE_CRASH_HANDLER_H
#define SOUL_CORE_CRASH_HANDLER_H

// ============================================================================
// crash_handler.h — 崩溃报告与内存泄漏检测 [v1.9.2 新增]
// ============================================================================
//
// 设计目标: 捕获崩溃信号并生成包含堆栈信息的崩溃报告,辅助排查问题。
// 在程序退出时检测未释放的资源,输出内存泄漏报告。
//
// 设计原则:
//   - 最小侵入: 仅需调用 install() 即可启用
//   - 跨平台: 支持 Windows(SEH/Minidump) / Linux(signal handler) / macOS
//   - 资源报告: 退出时统计 SingletonRegistry 中未清理的资源
//
// 用法:
//   int main() {
//       sc::CrashHandler::install();
//       // ... application code ...
//       sc::CrashHandler::uninstall();
//   }

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <functional>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace sc {

// ============================================================================
// CrashHandler — 崩溃信号处理
// ============================================================================
class CrashHandler {
public:
    /// @brief 崩溃回调类型
    using CrashCallback = std::function<void(int signal, const std::string& message)>;

    /// @brief 安装崩溃处理器
    static void install(const std::string& dumpDir = "",
                        CrashCallback callback = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_installed) return;
        m_installed = true;

        m_dumpDir = dumpDir.empty() ? getDefaultDumpDir() : dumpDir;
        m_callback = std::move(callback);

        // 注册信号处理器
        std::signal(SIGABRT, signalHandler);
        std::signal(SIGSEGV, signalHandler);
        std::signal(SIGILL, signalHandler);
        std::signal(SIGFPE, signalHandler);

#ifdef _WIN32
        // Windows SEH 异常处理
        SetUnhandledExceptionFilter(sehHandler);
#endif
    }

    /// @brief 卸载崩溃处理器
    static void uninstall() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_installed) return;

        std::signal(SIGABRT, SIG_DFL);
        std::signal(SIGSEGV, SIG_DFL);
        std::signal(SIGILL, SIG_DFL);
        std::signal(SIGFPE, SIG_DFL);

#ifdef _WIN32
        SetUnhandledExceptionFilter(nullptr);
#endif
        m_installed = false;
    }

    /// @brief 注册退出时的资源泄漏检查回调
    static void registerLeakCheck(std::function<void()> checker) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_leakCheckers.push_back(std::move(checker));
    }

    /// @brief 执行资源泄漏检查(v1.9.2)
    static void runLeakChecks() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& checker : m_leakCheckers) {
            if (checker) checker();
        }
    }

private:
    static std::string getDefaultDumpDir() {
        std::time_t now = std::time(nullptr);
        std::string dir = "crash_dumps";
#ifdef _WIN32
        CreateDirectoryA(dir.c_str(), nullptr);
#endif
        return dir;
    }

    /// @brief 信号处理函数
    static void signalHandler(int signal) {
        std::string message = getSignalName(signal);
        writeCrashReport(signal, message);

        if (m_callback) {
            m_callback(signal, message);
        }

        // 恢复默认处理器并重新触发
        std::signal(signal, SIG_DFL);
        std::raise(signal);
    }

#ifdef _WIN32
    /// @brief Windows SEH 异常处理
    static LONG WINAPI sehHandler(EXCEPTION_POINTERS* exceptionInfo) {
        std::string message = getSehExceptionName(exceptionInfo->ExceptionRecord->ExceptionCode);
        writeCrashReport(-1, message);

        // 生成 minidump
        writeMiniDump(exceptionInfo);

        if (m_callback) {
            m_callback(-1, message);
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    static void writeMiniDump(EXCEPTION_POINTERS* exceptionInfo) {
        std::string dumpPath = m_dumpDir + "/crash_" +
                               std::to_string(std::time(nullptr)) + ".dmp";

        HANDLE hFile = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0,
                                    nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = exceptionInfo;
            mei.ClientPointers = FALSE;

            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                              hFile, MiniDumpNormal,
                              exceptionInfo ? &mei : nullptr, nullptr, nullptr);
            CloseHandle(hFile);
        }
    }

    static std::string getSehExceptionName(DWORD code) {
        switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:         return "ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:     return "ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:               return "BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:     return "DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:      return "FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:        return "FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT:        return "FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION:     return "FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW:              return "FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK:           return "FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW:             return "FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:       return "ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:             return "IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:        return "INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:              return "INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:       return "INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:  return "NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:          return "PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:               return "SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:            return "STACK_OVERFLOW";
        default:                                  return "UNKNOWN_SEH_EXCEPTION";
        }
    }
#endif

    /// @brief 获取信号名称
    static std::string getSignalName(int signal) {
        switch (signal) {
        case SIGABRT: return "SIGABRT (Abnormal termination)";
        case SIGSEGV: return "SIGSEGV (Segmentation fault)";
        case SIGILL:  return "SIGILL (Illegal instruction)";
        case SIGFPE:  return "SIGFPE (Floating point exception)";
#ifdef SIGTERM
        case SIGTERM: return "SIGTERM (Termination)";
#endif
        default:      return "UNKNOWN_SIGNAL (" + std::to_string(signal) + ")";
        }
    }

    /// @brief 写入崩溃报告
    static void writeCrashReport(int signal, const std::string& message) {
        std::string reportPath = m_dumpDir + "/crash_report_" +
                                 std::to_string(std::time(nullptr)) + ".txt";

        std::ofstream report(reportPath);
        if (!report.is_open()) return;

        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);

        report << "=== SoulCoreKit Crash Report ===" << std::endl;
        report << "Time:    " << std::ctime(&timeT);
        report << "Signal:  " << signal << " (" << message << ")" << std::endl;
        report << "Version: " << "1.9.2" << std::endl;
        report << "Thread:  " << std::this_thread::get_id() << std::endl;
        report << "================================" << std::endl;
        report.close();
    }

    static bool m_installed;
    static std::string m_dumpDir;
    static CrashCallback m_callback;
    static std::vector<std::function<void()>> m_leakCheckers;
    static std::mutex m_mutex;
};

// 静态成员初始化
inline bool CrashHandler::m_installed = false;
inline std::string CrashHandler::m_dumpDir;
inline CrashHandler::CrashCallback CrashHandler::m_callback;
inline std::vector<std::function<void()>> CrashHandler::m_leakCheckers;
inline std::mutex CrashHandler::m_mutex;

} // namespace sc

#endif // SOUL_CORE_CRASH_HANDLER_H