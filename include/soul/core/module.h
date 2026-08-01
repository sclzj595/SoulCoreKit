#ifndef SOUL_CORE_MODULE_H
#define SOUL_CORE_MODULE_H

#include <string>
#include <vector>
#include "soul/core/result.h"

namespace sc {

// ============================================================================
// Module — 脚手架模块基类
// ============================================================================
//
// 对标 SpringBoot 的 @Component / @Configuration 生命周期模型。
// Scaffold 会按依赖顺序 + 优先级调用各模块的 init/start/stop/cleanup。
//
// 生命周期(对标 SpringBoot):
//   init()   -> @PostConstruct   资源准备(配置加载、依赖注入注册)
//   start()  -> ContextRefreshed 服务启动(网络监听、定时任务开启)
//   stop()   -> ContextClosed    服务停止(停止接收新请求)
//   cleanup()-> @PreDestroy      资源释放(关闭连接、释放内存)
//
// 依赖声明:
//   dependsOn() 返回依赖的模块名称列表,Scaffold 会拓扑排序后执行。
//
// 优先级:
//   priority() 返回优先级(越大越先 init,越晚 cleanup)。默认 0。
//   同优先级按注册顺序。依赖关系优先于优先级。
//
// 条件装配(对标 @ConditionalOnProperty):
//   isEnabled() 默认 true,子类可重写以根据配置决定是否装配。
class Module {
public:
    explicit Module(const std::string& name) : m_name(name) {}
    virtual ~Module() = default;

    // --- 基本信息 ---
    const std::string& name() const { return m_name; }

    // --- 生命周期钩子(子类按需重写) ---

    // 初始化阶段:资源准备、依赖注入注册。返回失败会触发回滚。
    virtual Result<void> init() { return {}; }

    // 启动阶段:服务启动、网络监听、定时任务开启。返回失败会触发回滚。
    // 默认实现空,兼容仅需要 init/cleanup 的简单模块。
    virtual Result<void> onStart() { return {}; }

    // 停止阶段:停止接收新请求、优雅停机。默认空。
    virtual void onStop() {}

    // 清理阶段:资源释放、关闭连接。默认空。
    // 与 init() 配对,无论 start() 是否执行,只要 init() 成功就会调用 cleanup()。
    virtual void cleanup() {}

    // --- 依赖声明与排序 ---

    // 声明依赖的模块名称列表。Scaffold 会按拓扑序初始化。
    // 默认无依赖。子类可重写以声明依赖。
    virtual std::vector<std::string> dependsOn() const { return {}; }

    // 优先级:越大越先 init、越晚 cleanup。默认 0。
    // 注意:依赖关系优先于优先级。被依赖的模块总是先 init。
    virtual int priority() const { return 0; }

    // --- 条件装配 ---

    // 是否启用此模块。默认 true。子类可重写以根据配置决定。
    // 返回 false 时,Scaffold 会跳过此模块(不 init/start/stop/cleanup)。
    virtual bool isEnabled() const { return true; }

private:
    std::string m_name;
};

} // namespace sc

#endif // SOUL_CORE_MODULE_H
