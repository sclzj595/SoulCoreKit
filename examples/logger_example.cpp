#include <memory>
#include "soul/logging/log_macros.h"
#include "soul/logging/file_sink.h"

int main() {
    sc::Logger::instance().setLogLevel(sc::LogLevel::Trace);

    auto fileSink = std::make_shared<sc::FileSink>("logs/soul_core.log");
    sc::Logger::instance().addSink(fileSink);

    SC_TRACE("This is a trace message");
    SC_DEBUG("This is a debug message");
    SC_INFO("This is an info message");
    SC_WARN("This is a warning message");
    SC_ERROR("This is an error message");

    SC_INFO_S("Network", "connect", "Connected to server");
    SC_WARN_S("Storage", "cache", "Cache miss for key user_123");
    SC_ERROR_S("Database", "query", "Failed to execute query");

    return 0;
}