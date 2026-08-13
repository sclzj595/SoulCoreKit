// ============================================================================
// cli_main.cpp — CLI / Headless 工具示例 [v2.7.0]
// ============================================================================
//
// 演示 SoulCoreKit 作为 CLI 工具或 Headless 服务的最小依赖场景:
//   - 仅 Core 层 (无 UI/Network/Server)
//   - Result<T>/Error 错误处理
//   - DI Container
//   - Logger
//   - 异步任务
//
// 构建:
//   cd examples/cli && mkdir build && cd build
//   cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt6
//   cmake --build .
//   ./cli_tool --help

#include "soul/soul.h"

using namespace sc;

// ============================================================================
// CLI 工具模块
// ============================================================================

class CliToolModule : public Module {
public:
    CliToolModule() : Module("CliTool") {}

    Result<void> init() override {
        SC_INFO("CLI tool initializing...");

        // 示例: 使用 Core 能力
        std::string id = Uuid::generate();
        SC_INFO_FMT("Generated UUID: {}", id);

        // 示例: Result<T> 错误处理
        auto result = doSomething();
        if (result.isErr()) {
            SC_ERROR_FMT("Operation failed: {}", result.unwrapErr().message().toStdString());
            return Result<void>::err(result.unwrapErr());  // 传播错误
        }

        SC_INFO_FMT("Operation result: {}", result.unwrap());
        return {};
    }

    Result<void> onStart() override {
        SC_INFO("CLI tool started");
        // CLI 工具通常在 onStart 中执行主逻辑并退出
        QCoreApplication::quit();
        return {};
    }

    void onStop() override {
        SC_INFO("CLI tool stopping");
    }

    void cleanup() override {
        SC_INFO("CLI tool shut down");
    }

private:
    Result<int> doSomething() {
        // 模拟可能失败的操作
        if (true) {  // 实际根据业务逻辑判断
            return Result<int>::ok(42);
        }
        return Result<int>::err(Error(ErrorCode::InternalError, "Something went wrong"));
    }
};

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[]) {
    CliToolModule cliModule;
    Scaffold scaffold(argc, argv);
    scaffold.use(cliModule);
    return scaffold.run();
}
