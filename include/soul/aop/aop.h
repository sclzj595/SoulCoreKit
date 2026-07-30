#ifndef SOUL_AOP_AOP_H
#define SOUL_AOP_AOP_H

// ============================================================================
// aop.h — AOP 切面编程模块(对标 SpringBoot AOP)
// ============================================================================
//
// 设计目标: 提供函数式 AOP 能力,对标 SpringBoot 的 @Aspect/@Pointcut/
// @Around/@Before/@After。由于 C++ 不支持注解,采用函数式封装:
//
//   - JoinPoint: 封装一次方法调用(目标函数 + 参数 + 返回值)
//   - Advice: 切面动作(Before/After/Around/AfterReturning/AfterThrowing)
//   - Pointcut: 连接点匹配器(按函数名/签名匹配)
//   - Aspect: 切面容器,持有一组 Advice + Pointcut
//   - AspectWeaver: 织入器,对目标函数应用匹配的切面
//
// 设计原则(遵循 project_memory 硬约束):
//   - C++17 严格限定,严禁协程
//   - RAII + 智能指针,严禁裸指针
//   - 单一职责: AOP 模块仅负责切面织入,不参与业务逻辑
//   - 线程安全: AspectWeaver 注册/查找线程安全
//   - 零侵入: 不修改目标类,通过包装函数织入
//
// 用法(Before/After):
//   sc::aop::Aspect aspect("logging");
//   aspect.setPointcut(sc::aop::Pointcut::byName("UserService::"));
//   aspect.setBefore([](const sc::aop::JoinPoint& jp) {
//       SC_INFO("Before: " + jp.methodName());
//   });
//   aspect.setAfter([](const sc::aop::JoinPoint& jp) {
//       SC_INFO("After: " + jp.methodName());
//   });
//   sc::aop::AspectWeaver::instance().registerAspect(aspect);
//
//   // 织入目标函数
//   auto result = sc::aop::AspectWeaver::instance().weave(
//       "UserService::login",
//       [](sc::aop::JoinPoint& jp) -> sc::aop::JoinPoint::ReturnType {
//           return doLogin(jp.arg<std::string>(0), jp.arg<std::string>(1));
//       },
//       {username, password}
//   );
//
// 用法(Around - 完全控制是否执行目标):
//   aspect.setAround([](sc::aop::JoinPoint& jp,
//                       const sc::aop::JoinPoint::ProceedFunc& proceed) -> sc::aop::JoinPoint::ReturnType {
//       auto start = std::chrono::steady_clock::now();
//       auto result = proceed(jp);  // 执行目标方法
//       auto elapsed = std::chrono::steady_clock::now() - start;
//       SC_INFO(jp.methodName() + " took " + std::to_string(elapsed.count()) + "ns");
//       return result;
//   });

#include <any>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {
namespace aop {

// ============================================================================
// JoinPoint — 连接点(封装一次方法调用)
// ============================================================================
//
// 持有方法名、参数列表、返回值。Around advice 通过 proceed() 执行目标方法。
class JoinPoint {
public:
    using ReturnType = std::any;
    using ProceedFunc = std::function<ReturnType(JoinPoint&)>;

    /// @param methodName 方法全名(如 "UserService::login")
    /// @param args       参数列表(用 std::any 类型擦除)
    JoinPoint(std::string methodName, std::vector<std::any> args)
        : m_methodName(std::move(methodName)), m_args(std::move(args)) {}

    /// @return 方法名
    const std::string& methodName() const noexcept { return m_methodName; }

    /// @return 参数数量
    std::size_t argCount() const noexcept { return m_args.size(); }

    /// @brief 获取第 index 个参数(类型由调用方指定)
    /// @tparam T 参数类型
    /// @throws std::bad_any_cast 若类型不匹配
    template<typename T>
    T arg(std::size_t index) const {
        return std::any_cast<T>(m_args.at(index));
    }

    /// @brief 设置返回值(Around advice 执行 proceed 后设置)
    void setReturnValue(ReturnType value) { m_returnValue = std::move(value); }

    /// @brief 获取返回值
    const ReturnType& returnValue() const noexcept { return m_returnValue; }

    /// @return 是否有异常(AfterThrowing advice 使用)
    bool hasException() const noexcept { return m_hasException; }

    /// @brief 标记有异常(由 weave 内部 try-catch 设置)
    void setException(bool has) noexcept { m_hasException = has; }

    /// @return 异常信息(若有)
    const std::string& exceptionMessage() const noexcept { return m_exceptionMsg; }

    /// @brief 设置异常信息
    void setExceptionMessage(std::string msg) { m_exceptionMsg = std::move(msg); }

private:
    std::string             m_methodName;
    std::vector<std::any>   m_args;
    ReturnType              m_returnValue;
    bool                    m_hasException = false;
    std::string             m_exceptionMsg;
};

// ============================================================================
// AdviceType — 切面动作类型
// ============================================================================
enum class AdviceType {
    Before,         ///< 方法执行前
    After,          ///< 方法执行后(无论成功失败)
    AfterReturning, ///< 方法成功返回后
    AfterThrowing,  ///< 方法抛异常后
    Around          ///< 环绕(完全控制是否执行目标)
};

// ============================================================================
// Advice 回调类型定义
// ============================================================================
using BeforeFunc        = std::function<void(const JoinPoint&)>;
using AfterFunc         = std::function<void(const JoinPoint&)>;
using AfterReturningFunc= std::function<void(const JoinPoint&)>;
using AfterThrowingFunc = std::function<void(const JoinPoint&)>;
using AroundFunc        = std::function<JoinPoint::ReturnType(JoinPoint&, const JoinPoint::ProceedFunc&)>;

// ============================================================================
// Pointcut — 连接点匹配器
// ============================================================================
//
// 按方法名前缀/后缀/包含/正则匹配。对标 SpringBoot 的 @Pointcut 表达式。
class Pointcut {
public:
    enum class MatchMode {
        Prefix,     ///< 前缀匹配(如 "UserService::" 匹配所有 UserService 方法)
        Suffix,     ///< 后缀匹配
        Contains,   ///< 包含匹配
        Exact       ///< 精确匹配
    };

    /// @brief 创建前缀匹配 Pointcut
    static Pointcut byPrefix(std::string prefix) {
        return Pointcut(MatchMode::Prefix, std::move(prefix));
    }

    /// @brief 创建包含匹配 Pointcut
    static Pointcut byContains(std::string substr) {
        return Pointcut(MatchMode::Contains, std::move(substr));
    }

    /// @brief 创建精确匹配 Pointcut
    static Pointcut byExact(std::string name) {
        return Pointcut(MatchMode::Exact, std::move(name));
    }

    /// @brief 创建后缀匹配 Pointcut
    static Pointcut bySuffix(std::string suffix) {
        return Pointcut(MatchMode::Suffix, std::move(suffix));
    }

    /// @brief 匹配方法名
    bool matches(const std::string& methodName) const {
        switch (m_mode) {
            case MatchMode::Prefix:
                return methodName.size() >= m_pattern.size()
                    && methodName.compare(0, m_pattern.size(), m_pattern) == 0;
            case MatchMode::Suffix:
                return methodName.size() >= m_pattern.size()
                    && methodName.compare(methodName.size() - m_pattern.size(),
                                          m_pattern.size(), m_pattern) == 0;
            case MatchMode::Contains:
                return methodName.find(m_pattern) != std::string::npos;
            case MatchMode::Exact:
                return methodName == m_pattern;
        }
        return false;
    }

    MatchMode mode() const noexcept { return m_mode; }
    const std::string& pattern() const noexcept { return m_pattern; }

private:
    Pointcut(MatchMode mode, std::string pattern)
        : m_mode(mode), m_pattern(std::move(pattern)) {}

    MatchMode   m_mode;
    std::string m_pattern;
};

// ============================================================================
// Aspect — 切面容器
// ============================================================================
//
// 持有一个 Pointcut 和一组 Advice。对标 SpringBoot 的 @Aspect 注解类。
// 一个 Aspect 可同时配置 Before/After/Around 等多种 Advice。
class Aspect {
public:
    /// @param name 切面名称(唯一标识)
    explicit Aspect(std::string name) : m_name(std::move(name)) {}

    const std::string& name() const noexcept { return m_name; }

    /// @brief 设置连接点匹配器
    void setPointcut(Pointcut pointcut) { m_pointcut = std::move(pointcut); }

    /// @return 是否已配置 Pointcut
    bool hasPointcut() const noexcept { return m_pointcut.has_value(); }

    /// @brief 匹配方法名
    bool matches(const std::string& methodName) const {
        return m_pointcut ? m_pointcut->matches(methodName) : false;
    }

    // Advice 设置(Before/After/AfterReturning/AfterThrowing/Around 各最多一个)
    void setBefore(BeforeFunc func) { m_before = std::move(func); }
    void setAfter(AfterFunc func) { m_after = std::move(func); }
    void setAfterReturning(AfterReturningFunc func) { m_afterReturning = std::move(func); }
    void setAfterThrowing(AfterThrowingFunc func) { m_afterThrowing = std::move(func); }
    void setAround(AroundFunc func) { m_around = std::move(func); }

    bool hasBefore() const noexcept { return static_cast<bool>(m_before); }
    bool hasAfter() const noexcept { return static_cast<bool>(m_after); }
    bool hasAfterReturning() const noexcept { return static_cast<bool>(m_afterReturning); }
    bool hasAfterThrowing() const noexcept { return static_cast<bool>(m_afterThrowing); }
    bool hasAround() const noexcept { return static_cast<bool>(m_around); }

    const BeforeFunc& before() const noexcept { return m_before; }
    const AfterFunc& after() const noexcept { return m_after; }
    const AfterReturningFunc& afterReturning() const noexcept { return m_afterReturning; }
    const AfterThrowingFunc& afterThrowing() const noexcept { return m_afterThrowing; }
    const AroundFunc& around() const noexcept { return m_around; }

private:
    std::string                 m_name;
    std::optional<Pointcut>     m_pointcut;
    BeforeFunc                  m_before;
    AfterFunc                   m_after;
    AfterReturningFunc          m_afterReturning;
    AfterThrowingFunc           m_afterThrowing;
    AroundFunc                  m_around;
};

// ============================================================================
// AspectWeaver — 织入器(单例)
// ============================================================================
//
// 全局注册中心,管理所有 Aspect。weave() 方法对目标函数应用匹配的切面。
//
// 织入顺序(对标 SpringBoot):
//   1. Before advice(按注册顺序)
//   2. Around advice 的前半部分
//   3. 目标方法执行(proceed)
//   4. Around advice 的后半部分
//   5. AfterReturning advice(若成功)/ AfterThrowing advice(若异常)
//   6. After advice(无论成功失败)
//
// @thread_safety Thread-Safe
class AspectWeaver {
public:
    /// @brief 获取单例
    static AspectWeaver& instance();

    /// @brief 注册切面(同名覆盖)
    void registerAspect(Aspect aspect);

    /// @brief 注销切面
    void unregisterAspect(const std::string& name);

    /// @brief 清空所有切面(仅用于测试)
    void clear();

    /// @return 已注册切面数量
    std::size_t aspectCount() const;

    /// @brief 织入切面并执行目标函数
    /// @param methodName 方法名(用于 Pointcut 匹配)
    /// @param target     目标函数(接收 JoinPoint,返回 ReturnType)
    /// @param args       参数列表(类型擦除)
    /// @return 目标函数返回值(可能被 Around advice 修改)
    JoinPoint::ReturnType weave(const std::string& methodName,
                                  JoinPoint::ProceedFunc target,
                                  std::vector<std::any> args = {});

private:
    AspectWeaver() = default;
    AspectWeaver(const AspectWeaver&) = delete;
    AspectWeaver& operator=(const AspectWeaver&) = delete;

    mutable std::mutex m_mutex;
    std::vector<Aspect> m_aspects;
};

} // namespace aop
} // namespace sc

#endif // SOUL_AOP_AOP_H
