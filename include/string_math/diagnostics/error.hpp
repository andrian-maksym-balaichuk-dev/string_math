#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
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

} // namespace string_math
