// ============================================================================
// aop.cpp — AOP 切面编程实现
// ============================================================================
//
// 实现 AspectWeaver 单例的注册/注销/weave 织入逻辑。
// weave 织入顺序对标 SpringBoot:
//   Before -> Around前半 -> 目标方法 -> Around后半 -> AfterReturning/AfterThrowing -> After

#include "soul/aop/aop.h"
#include "soul/logging/log_macros.h"

#include <algorithm>
#include <stdexcept>

namespace sc {
namespace aop {

// ============================================================================
// AspectWeaver 实现
// ============================================================================

AspectWeaver& AspectWeaver::instance() {
    static AspectWeaver s_instance;
    return s_instance;
}

void AspectWeaver::registerAspect(Aspect aspect) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 同名覆盖:先移除旧的
    const std::string& name = aspect.name();
    m_aspects.erase(
        std::remove_if(m_aspects.begin(), m_aspects.end(),
            [&name](const Aspect& a) { return a.name() == name; }),
        m_aspects.end());
    m_aspects.push_back(std::move(aspect));
}

void AspectWeaver::unregisterAspect(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aspects.erase(
        std::remove_if(m_aspects.begin(), m_aspects.end(),
            [&name](const Aspect& a) { return a.name() == name; }),
        m_aspects.end());
}

void AspectWeaver::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aspects.clear();
}

std::size_t AspectWeaver::aspectCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_aspects.size();
}

JoinPoint::ReturnType AspectWeaver::weave(const std::string& methodName,
                                            JoinPoint::ProceedFunc target,
                                            std::vector<std::any> args) {
    // 1. 收集匹配的切面(快照,避免持锁执行 advice)
    std::vector<Aspect> matchedAspects;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& aspect : m_aspects) {
            if (aspect.matches(methodName)) {
                matchedAspects.push_back(aspect);
            }
        }
    }

    // 2. 构造 JoinPoint
    JoinPoint jp(methodName, std::move(args));

    // 3. Before advice(按注册顺序)
    for (auto& aspect : matchedAspects) {
        if (aspect.hasBefore() && aspect.before()) {
            aspect.before()(jp);
        }
    }

    // 4. 执行目标方法(可能被 Around 包装)
    JoinPoint::ReturnType result;
    // 用 exception_ptr 捕获原始异常,保留类型信息(对标 SpringBoot 的 throws 语义)
    std::exception_ptr eptr;

    try {
        // 构造最内层的 proceed 函数(执行目标)
        JoinPoint::ProceedFunc innerProceed = std::move(target);

        // 从内到外包装 Around advice(后注册的 Around 在最外层)
        // 这样 Around 的前半按注册顺序执行,后半按逆序执行
        for (auto it = matchedAspects.rbegin(); it != matchedAspects.rend(); ++it) {
            auto& aspect = *it;
            if (!aspect.hasAround() || !aspect.around()) {
                continue;
            }
            auto aroundFunc = aspect.around();
            auto captured = std::move(innerProceed);
            innerProceed = [aroundFunc, captured](JoinPoint& innerJp) -> JoinPoint::ReturnType {
                return aroundFunc(innerJp, captured);
            };
        }

        if (innerProceed) {
            result = innerProceed(jp);
            jp.setReturnValue(result);
        }
    } catch (...) {
        // Blanket catch: AOP 织入器必须捕获所有异常,避免异常逃逸到 advice 阶段。
        // 通过 std::current_exception() 保留原始异常类型,最终在步骤 7 原样重抛。
        eptr = std::current_exception();
        jp.setException(true);
        // 提取 what() 供 AfterThrowing advice 的 exceptionMessage() 使用
        try {
            std::rethrow_exception(eptr);
        } catch (const std::exception& e) {
            jp.setExceptionMessage(e.what());
        } catch (...) { // Blanket catch: capture exception type unknown at compile time
            jp.setExceptionMessage("unknown exception");
        }
    }

    const bool hasException = (eptr != nullptr);

    // 5. AfterReturning / AfterThrowing advice
    if (!hasException) {
        for (auto& aspect : matchedAspects) {
            if (aspect.hasAfterReturning() && aspect.afterReturning()) {
                aspect.afterReturning()(jp);
            }
        }
    } else {
        for (auto& aspect : matchedAspects) {
            if (aspect.hasAfterThrowing() && aspect.afterThrowing()) {
                aspect.afterThrowing()(jp);
            }
        }
    }

    // 6. After advice(无论成功失败)
    for (auto& aspect : matchedAspects) {
        if (aspect.hasAfter() && aspect.after()) {
            aspect.after()(jp);
        }
    }

    // 7. 若有异常,原样重抛(保留原始异常类型,保持目标方法异常语义)
    if (eptr) {
        std::rethrow_exception(eptr);
    }

    return result;
}

} // namespace aop
} // namespace sc
