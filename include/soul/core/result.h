#ifndef SOUL_CORE_RESULT_H
#define SOUL_CORE_RESULT_H

#include <variant>
#include <string>
#include <optional>
#include <stdexcept>

#include "soul/core/error.h"

namespace sc {

template<typename T>
class Result;

/**
 * @class ResultVoid
 * @brief 用于 Result<void> 的基类
 *
 * ResultVoid 提供了 void 类型 Result 的基础功能，包括错误状态检查和错误获取。
 * 这是一个内部类，用户应直接使用 Result<void>。
 */
class ResultVoid {
public:
    /**
     * @brief 默认构造函数，创建成功状态
     */
    ResultVoid() = default;

    /**
     * @brief 从 Error 创建失败状态
     * @param error 错误对象
     */
    ResultVoid(const Error& error) : m_error(error), m_isErr(true) {}

    /**
     * @brief 从 Error 移动构造失败状态
     * @param error 错误对象（右值）
     */
    ResultVoid(Error&& error) : m_error(std::move(error)), m_isErr(true) {}

    /**
     * @brief 检查是否为成功状态
     * @return 如果成功返回 true
     */
    bool isOk() const { return !m_isErr; }

    /**
     * @brief 检查是否为失败状态
     * @return 如果失败返回 true
     */
    bool isErr() const { return m_isErr; }

    /**
     * @brief 获取错误对象（非 const）
     * @return 错误对象引用
     * @throw std::runtime_error 如果状态为成功
     */
    Error& unwrapErr() {
        if (!m_isErr) throw std::runtime_error("Result is Ok");
        return m_error;
    }

    /**
     * @brief 获取错误对象（const）
     * @return 错误对象 const 引用
     * @throw std::runtime_error 如果状态为成功
     */
    const Error& unwrapErr() const {
        if (!m_isErr) throw std::runtime_error("Result is Ok");
        return m_error;
    }

private:
    Error m_error;
    bool m_isErr = false;
};

/**
 * @class Result
 * @brief Rust 风格的结果类型
 *
 * Result<T> 封装了操作的结果，可以是成功时的值 T 或失败时的 Error。
 * 提供链式操作支持，如 map() 和 andThen()。
 *
 * @tparam T 成功时的值类型
 *
 * @code
 * // 创建成功结果
 * Result<int> success(42);
 *
 * // 创建失败结果
 * Result<int> failure(Error(ErrorCode::NotFound, "Not found"));
 *
 * // 链式操作
 * auto result = someOperation()
 *     .map([](int v) { return v * 2; })
 *     .andThen([](int v) { return anotherOperation(v); });
 * @endcode
 */
template<typename T>
class Result {
public:
    /**
     * @brief 默认构造函数，创建空结果
     */
    Result() = default;

    /**
     * @brief 从值构造成功结果（拷贝）
     * @param value 成功时的值
     */
    Result(const T& value) : m_data(value) {}

    /**
     * @brief 从值构造成功结果（移动）
     * @param value 成功时的值（右值）
     */
    Result(T&& value) : m_data(std::move(value)) {}

    /**
     * @brief 从 Error 构造失败结果（拷贝）
     * @param error 错误对象
     */
    Result(const Error& error) : m_data(error) {}

    /**
     * @brief 从 Error 构造失败结果（移动）
     * @param error 错误对象（右值）
     */
    Result(Error&& error) : m_data(std::move(error)) {}

    /**
     * @brief 检查是否为成功状态
     * @return 如果成功返回 true
     */
    bool isOk() const { return std::holds_alternative<T>(m_data); }

    /**
     * @brief 检查是否为失败状态
     * @return 如果失败返回 true
     */
    bool isErr() const { return std::holds_alternative<Error>(m_data); }

    /**
     * @brief 获取成功值（非 const）
     * @return 值的引用
     * @throw std::runtime_error 如果状态为失败
     */
    T& unwrap() {
        if (isErr()) throw std::runtime_error("Result is Err");
        return std::get<T>(m_data);
    }

    /**
     * @brief 获取成功值（const）
     * @return 值的 const 引用
     * @throw std::runtime_error 如果状态为失败
     */
    const T& unwrap() const {
        if (isErr()) throw std::runtime_error("Result is Err");
        return std::get<T>(m_data);
    }

    /**
     * @brief 获取错误对象（非 const）
     * @return 错误对象引用
     * @throw std::runtime_error 如果状态为成功
     */
    Error& unwrapErr() {
        if (isOk()) throw std::runtime_error("Result is Ok");
        return std::get<Error>(m_data);
    }

    /**
     * @brief 获取错误对象（const）
     * @return 错误对象 const 引用
     * @throw std::runtime_error 如果状态为成功
     */
    const Error& unwrapErr() const {
        if (isOk()) throw std::runtime_error("Result is Ok");
        return std::get<Error>(m_data);
    }

    /**
     * @brief 获取值或默认值
     * @param defaultValue 默认值
     * @return 成功时返回实际值，失败时返回默认值
     */
    T valueOrDefault(const T& defaultValue) const {
        if (isOk()) return std::get<T>(m_data);
        return defaultValue;
    }

    /**
     * @brief 映射值到新类型
     * @tparam F 映射函数类型
     * @param func 映射函数
     * @return 映射后的 Result<U>
     */
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))> {
        using U = decltype(func(std::declval<T>()));
        if (isOk()) {
            return Result<U>(func(std::get<T>(m_data)));
        }
        return Result<U>(std::get<Error>(m_data));
    }

    /**
     * @brief 链式操作，将值传递给返回 Result 的函数
     * @tparam F 函数类型
     * @param func 返回 Result 的函数
     * @return func 执行后的 Result
     */
    template<typename F>
    auto andThen(F&& func) const -> decltype(func(std::declval<T>())) {
        if (isOk()) {
            return func(std::get<T>(m_data));
        }
        using U = decltype(func(std::declval<T>()));
        return U(std::get<Error>(m_data));
    }

private:
    std::variant<T, Error> m_data;
};

/**
 * @brief Result<void> 的特化版本
 *
 * void 类型的 Result，用于表示只关心成功与否的操作结果。
 */
template<>
class Result<void> : public ResultVoid {
public:
    using ResultVoid::ResultVoid;
};

}

#endif
