#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace string_math
{

enum class ErrorKind
{
    Parse,
    Literal,
    NameResolution,
    OverloadResolution,
    Evaluation,
    Internal
};

struct SourceSpan
{
    std::size_t begin{0};
    std::size_t end{0};
};

struct Diagnostic
{
    ErrorKind kind{ErrorKind::Internal};
    std::string message;
    SourceSpan span{};
    std::string token;
    std::vector<std::string> expected;
};

class Error
{
public:
    Error() = default;

    explicit Error(Diagnostic diagnostic) : m_diagnostic(std::move(diagnostic)) {}

    Error(
        ErrorKind kind,
        std::string message,
        SourceSpan span = {},
        std::string token = {},
        std::vector<std::string> expected = {})
    : m_diagnostic{kind, std::move(message), span, std::move(token), std::move(expected)}
    {}

    const Diagnostic& diagnostic() const noexcept
    {
        return m_diagnostic;
    }

    ErrorKind kind() const noexcept
    {
        return m_diagnostic.kind;
    }

    const std::string& message() const noexcept
    {
        return m_diagnostic.message;
    }

    const SourceSpan& span() const noexcept
    {
        return m_diagnostic.span;
    }

    const std::string& token() const noexcept
    {
        return m_diagnostic.token;
    }

    const std::vector<std::string>& expected() const noexcept
    {
        return m_diagnostic.expected;
    }

    std::string to_string() const
    {
        std::string text = m_diagnostic.message;
        text += " [";
        text += std::to_string(m_diagnostic.span.begin);
        text += ",";
        text += std::to_string(m_diagnostic.span.end);
        text += ")";
        if (!m_diagnostic.token.empty())
        {
            text += " token='";
            text += m_diagnostic.token;
            text += "'";
        }
        if (!m_diagnostic.expected.empty())
        {
            text += " expected=";
            for (std::size_t index = 0; index < m_diagnostic.expected.size(); ++index)
            {
                if (index != 0)
                {
                    text += ",";
                }
                text += m_diagnostic.expected[index];
            }
        }
        return text;
    }

private:
    Diagnostic m_diagnostic{};
};

[[noreturn]] inline void throw_error(const Error& error)
{
    throw std::runtime_error(error.to_string());
}

// ---------------------------------------------------------------------------

template <class T>
class Result
{
public:
    constexpr Result(const T& value) : m_storage(value) {}
    constexpr Result(T&& value) : m_storage(std::move(value)) {}
    Result(const Error& error) : m_storage(error) {}
    Result(Error&& error) : m_storage(std::move(error)) {}

    constexpr bool has_value() const noexcept
    {
        return std::holds_alternative<T>(m_storage);
    }

    constexpr bool has_error() const noexcept
    {
        return !has_value();
    }

    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    constexpr T& value() &
    {
        if (has_error())
        {
            throw_error(error());
        }
        return std::get<T>(m_storage);
    }

    constexpr const T& value() const&
    {
        if (has_error())
        {
            throw_error(error());
        }
        return std::get<T>(m_storage);
    }

    constexpr T&& value() &&
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
