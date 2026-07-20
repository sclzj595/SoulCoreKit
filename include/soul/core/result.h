#ifndef SOUL_CORE_RESULT_H
#define SOUL_CORE_RESULT_H

#include <variant>
#include <string>
#include <optional>
#include <cassert>
#include <cstdlib>
#include <functional>

#include "soul/core/error.h"

namespace sc {

template<typename T>
class Result;

class ResultVoid {
public:
    ResultVoid() = default;

    ResultVoid(const Error& error) : m_error(error), m_isErr(true) {}

    ResultVoid(Error&& error) : m_error(std::move(error)), m_isErr(true) {}

    bool isOk() const { return !m_isErr; }

    bool isErr() const { return m_isErr; }

    Error& unwrapErr() {
        if (!m_isErr) {
            std::abort();
        }
        return m_error;
    }

    const Error& unwrapErr() const {
        if (!m_isErr) {
            std::abort();
        }
        return m_error;
    }

protected:
    Error m_error;
    bool m_isErr = false;
};

template<typename T>
class Result {
public:
    Result() = delete;

    Result(const T& value) : m_data(value) {}

    Result(T&& value) : m_data(std::move(value)) {}

    Result(const Error& error) : m_data(error) {}

    Result(Error&& error) : m_data(std::move(error)) {}

    static Result<T> ok(const T& value) {
        return Result<T>(value);
    }

    static Result<T> ok(T&& value) {
        return Result<T>(std::move(value));
    }

    static Result<T> err(const Error& error) {
        return Result<T>(error);
    }

    static Result<T> err(Error&& error) {
        return Result<T>(std::move(error));
    }

    bool isOk() const { return std::holds_alternative<T>(m_data); }

    bool isErr() const { return std::holds_alternative<Error>(m_data); }

    T& unwrap() {
        if (!isOk()) {
            std::abort();
        }
        return std::get<T>(m_data);
    }

    const T& unwrap() const {
        if (!isOk()) {
            std::abort();
        }
        return std::get<T>(m_data);
    }

    Error& unwrapErr() {
        if (!isErr()) {
            std::abort();
        }
        return std::get<Error>(m_data);
    }

    const Error& unwrapErr() const {
        if (!isErr()) {
            std::abort();
        }
        return std::get<Error>(m_data);
    }

    T valueOrDefault(const T& defaultValue) const {
        if (isOk()) return std::get<T>(m_data);
        return defaultValue;
    }

    T unwrapOr(T defaultValue) {
        if (isOk()) return std::get<T>(m_data);
        return std::move(defaultValue);
    }

    T unwrapOrElse(std::function<T(const Error&)> fn) {
        if (isOk()) return std::get<T>(m_data);
        return fn(std::get<Error>(m_data));
    }

    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<const T&>()))> {
        using U = decltype(func(std::declval<const T&>()));
        if (isOk()) {
            return Result<U>(func(std::get<T>(m_data)));
        }
        return Result<U>(std::get<Error>(m_data));
    }

    template<typename F>
    auto mapErr(F&& func) const -> Result<T> {
        if (isErr()) {
            return Result<T>(func(std::get<Error>(m_data)));
        }
        return Result<T>(std::get<T>(m_data));
    }

    template<typename F>
    auto andThen(F&& func) const -> decltype(func(std::declval<const T&>())) {
        if (isOk()) {
            return func(std::get<T>(m_data));
        }
        using U = decltype(func(std::declval<const T&>()));
        return U(std::get<Error>(m_data));
    }

    template<typename F>
    auto orElse(F&& func) const -> Result<T> {
        if (isErr()) {
            return func(std::get<Error>(m_data));
        }
        return Result<T>(std::get<T>(m_data));
    }

    std::optional<T> ok() const {
        if (isOk()) {
            return std::optional<T>(std::get<T>(m_data));
        }
        return std::nullopt;
    }

    std::optional<Error> err() const {
        if (isErr()) {
            return std::optional<Error>(std::get<Error>(m_data));
        }
        return std::nullopt;
    }

private:
    std::variant<T, Error> m_data;
};

template<>
class Result<void> : public ResultVoid {
public:
    using ResultVoid::ResultVoid;

    static Result<void> ok() {
        return Result<void>();
    }

    static Result<void> err(const Error& error) {
        return Result<void>(error);
    }

    static Result<void> err(Error&& error) {
        return Result<void>(std::move(error));
    }
};

}

#endif
