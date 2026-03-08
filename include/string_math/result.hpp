#pragma once

#include <utility>
#include <variant>

#include <string_math/error.hpp>

namespace string_math
{

template <class T>
class Result
{
public:
    Result(const T& value) : m_storage(value) {}
    Result(T&& value) : m_storage(std::move(value)) {}
    Result(const Error& error) : m_storage(error) {}
    Result(Error&& error) : m_storage(std::move(error)) {}

    bool has_value() const noexcept
    {
        return std::holds_alternative<T>(m_storage);
    }

    bool has_error() const noexcept
    {
        return !has_value();
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    T& value() &
    {
        if (has_error())
        {
            throw_error(error());
        }
        return std::get<T>(m_storage);
    }

    const T& value() const&
    {
        if (has_error())
        {
            throw_error(error());
        }
        return std::get<T>(m_storage);
    }

    T&& value() &&
    {
        if (has_error())
        {
            throw_error(error());
        }
        return std::get<T>(std::move(m_storage));
    }

    const Error& error() const&
    {
        return std::get<Error>(m_storage);
    }

    Error& error() &
    {
        return std::get<Error>(m_storage);
    }

private:
    std::variant<T, Error> m_storage;
};

} // namespace string_math
