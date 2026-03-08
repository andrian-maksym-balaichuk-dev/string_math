#pragma once

#include <functional>

#include <string_math/context.hpp>

namespace string_math
{

class Operation;
struct ParsedOperation;
struct BoundOperation;
Result<Operation> try_create_operation(std::string_view expression, const MathContext& context);
Result<MathValue> try_evaluate_operation(const Operation& operation, const MathContext& context);
Result<ParsedOperation> try_parse_operation(std::string_view expression, const MathContext& context);
Result<BoundOperation> try_bind_operation(const ParsedOperation& operation, FrozenMathContext context);
Result<Operation> try_optimize_operation(const BoundOperation& operation);

namespace detail
{

struct Node
{
    enum class Kind
    {
        Literal,
        Variable,
        PrefixOperator,
        PostfixOperator,
        InfixOperator,
        FunctionCall,
        Conditional
    };

    Kind kind{Kind::Literal};
    MathValue literal{};
    std::string text;
    int left{-1};
    int right{-1};
    int third{-1};
    int arity{0};
    std::vector<int> arguments;
    SourceSpan span{};
};

struct ParsedText
{
    std::string text;
    SourceSpan span{};
};

class Parser
{
public:
    Parser(std::string_view expression, const MathContext& context)
        : m_expression(expression),
          m_context(context)
    {}

    Result<Operation> try_parse();
    Operation parse();

private:
    void m_skip_spaces() noexcept
    {
        while (m_position < m_expression.size() && is_space(m_expression[m_position]))
        {
            ++m_position;
        }
    }

    Result<int> m_parse_expression(int min_precedence);
    Result<int> m_parse_prefix_or_primary();
    Result<int> m_parse_primary();
    Result<int> m_parse_function_call(ParsedText name);
    ParsedText m_parse_identifier();
    Result<ParsedText> m_parse_variable_name(std::size_t open_brace_position);
    ParsedText m_parse_literal_token();
    bool m_looks_like_literal_start() const noexcept;

    int m_add_node(Node node)
    {
        m_nodes.push_back(std::move(node));
        return static_cast<int>(m_nodes.size() - 1);
    }

    std::string_view m_tail() const noexcept
    {
        return m_expression.substr(m_position);
    }

    Error m_make_error(
        ErrorKind kind,
        std::string message,
        SourceSpan span,
        std::string token = {},
        std::vector<std::string> expected = {}) const
    {
        return Error(kind, std::move(message), span, std::move(token), std::move(expected));
    }

    Error m_make_error(
        ErrorKind kind,
        std::string message,
        std::size_t begin,
        std::size_t end,
        std::string token = {},
        std::vector<std::string> expected = {}) const
    {
        return m_make_error(kind, std::move(message), SourceSpan{begin, end}, std::move(token), std::move(expected));
    }

    std::string_view m_expression;
    const MathContext& m_context;
    std::size_t m_position{0};
    std::vector<Node> m_nodes;
    std::set<std::string> m_variable_names;
};

Operation make_subtree_operation(const Operation& source, const Node& node, std::vector<Node> child_nodes);
int fold_node(
    const Operation& source,
    const MathContext& context,
    int index,
    std::vector<Node>& optimized_nodes,
    std::unordered_map<int, int>& remapped_indices);

} // namespace detail

class Operation
{
public:
    Operation() = default;

    const std::vector<std::string>& variables() const noexcept
    {
        return m_variables;
    }

private:
    friend class detail::Parser;
    friend class Calculator;
    friend class MathExpr;
    friend Result<MathValue> try_evaluate_operation(const Operation& operation, const MathContext& context);
    friend MathValue evaluate_operation(const Operation& operation, const MathContext& context);
    friend Result<ParsedOperation> try_parse_operation(std::string_view expression, const MathContext& context);
    friend Result<BoundOperation> try_bind_operation(const ParsedOperation& operation, FrozenMathContext context);
    friend Result<Operation> try_optimize_operation(const BoundOperation& operation);
    friend Operation detail::make_subtree_operation(const Operation& source, const detail::Node& node, std::vector<detail::Node> child_nodes);
    friend int detail::fold_node(
        const Operation& source,
        const MathContext& context,
        int index,
        std::vector<detail::Node>& optimized_nodes,
        std::unordered_map<int, int>& remapped_indices);

    std::vector<detail::Node> m_nodes;
    int m_root{-1};
    std::vector<std::string> m_variables;
};

struct ParsedOperation
{
    Operation operation;
};

struct BoundOperation
{
    Operation operation;
    FrozenMathContext context;
};

inline Operation detail::Parser::parse()
{
    return try_parse().value();
}

inline Result<Operation> detail::Parser::try_parse()
{
    Operation operation;
    const auto root_result = m_parse_expression(0);
    if (!root_result)
    {
        return root_result.error();
    }

    operation.m_root = root_result.value();
    m_skip_spaces();
    if (m_position != m_expression.size())
    {
        return m_make_error(
            ErrorKind::Parse,
            "string_math: unexpected trailing input",
            m_position,
            m_expression.size(),
            std::string(m_expression.substr(m_position)));
    }

    operation.m_nodes = std::move(m_nodes);
    operation.m_variables.assign(m_variable_names.begin(), m_variable_names.end());
    return operation;
}

inline Result<ParsedOperation> try_parse_operation(std::string_view expression, const MathContext& context)
{
    const auto operation_result = detail::Parser(expression, context).try_parse();
    if (!operation_result)
    {
        return operation_result.error();
    }

    return ParsedOperation{operation_result.value()};
}

inline Result<BoundOperation> try_bind_operation(const ParsedOperation& operation, FrozenMathContext context)
{
    return BoundOperation{operation.operation, std::move(context)};
}

inline Result<int> detail::Parser::m_parse_expression(int min_precedence)
{
    const auto left_result = m_parse_prefix_or_primary();
    if (!left_result)
    {
        return left_result.error();
    }

    int left = left_result.value();

    for (;;)
    {
        m_skip_spaces();
        if (m_position >= m_expression.size() || m_expression[m_position] == ')' || m_expression[m_position] == ',')
        {
            break;
        }

        const std::size_t operator_position = m_position;
        const std::string postfix = m_context.match_postfix_symbol(m_tail());
        if (!postfix.empty())
        {
            const auto* entry = m_context.find_postfix_operator(postfix);
            if (entry != nullptr && entry->precedence >= min_precedence)
            {
                m_position += postfix.size();
                left = m_add_node(detail::Node{
                    detail::Node::Kind::PostfixOperator,
                    {},
                    postfix,
                    left,
                    -1,
                    -1,
                    1,
                    {},
                    SourceSpan{operator_position, operator_position + postfix.size()},
                });
                continue;
            }
        }

        const std::string infix = m_context.match_infix_symbol(m_tail());
        if (infix.empty())
        {
            break;
        }

        const auto* entry = m_context.find_infix_operator(infix);
        if (entry == nullptr || entry->precedence < min_precedence)
        {
            break;
        }

        m_position += infix.size();
        const int next_min =
            entry->associativity == Associativity::Left ? entry->precedence + 1 : entry->precedence;
        const auto right_result = m_parse_expression(next_min);
        if (!right_result)
        {
            return right_result.error();
        }

        left = m_add_node(detail::Node{
            detail::Node::Kind::InfixOperator,
            {},
            infix,
            left,
            right_result.value(),
            -1,
            2,
            {},
            SourceSpan{operator_position, operator_position + infix.size()},
        });
    }

    m_skip_spaces();
    if (min_precedence <= Precedence::Conditional && m_position < m_expression.size() && m_expression[m_position] == '?')
    {
        const std::size_t conditional_position = m_position++;
        const auto true_result = m_parse_expression(0);
        if (!true_result)
        {
            return true_result.error();
        }

        m_skip_spaces();
        if (m_position >= m_expression.size() || m_expression[m_position] != ':')
        {
            return m_make_error(
                ErrorKind::Parse,
                "string_math: expected ':' in conditional expression",
                m_position,
                m_position,
                {},
                {":"});
        }

        ++m_position;
        const auto false_result = m_parse_expression(Precedence::Conditional);
        if (!false_result)
        {
            return false_result.error();
        }

        left = m_add_node(detail::Node{
            detail::Node::Kind::Conditional,
            {},
            "?",
            left,
            true_result.value(),
            false_result.value(),
            3,
            {},
            SourceSpan{conditional_position, conditional_position + 1},
        });
    }

    return left;
}

inline Result<int> detail::Parser::m_parse_prefix_or_primary()
{
    m_skip_spaces();
    if (m_position >= m_expression.size())
    {
        return m_make_error(
            ErrorKind::Parse,
            "string_math: expected expression",
            m_position,
            m_position,
            {},
            {"expression"});
    }

    const std::size_t operator_position = m_position;
    const std::string symbol = m_context.match_prefix_symbol(m_tail());
    if (!symbol.empty())
    {
        const auto* entry = m_context.find_prefix_operator(symbol);
        if (entry != nullptr)
        {
            m_position += symbol.size();
            const auto operand_result = m_parse_expression(entry->precedence);
            if (!operand_result)
            {
                return operand_result.error();
            }

            return m_add_node(detail::Node{
                detail::Node::Kind::PrefixOperator,
                {},
                symbol,
                operand_result.value(),
                -1,
                -1,
                1,
                {},
                SourceSpan{operator_position, operator_position + symbol.size()},
            });
        }
    }

    return m_parse_primary();
}

inline Result<int> detail::Parser::m_parse_primary()
{
    m_skip_spaces();
    if (m_position >= m_expression.size())
    {
        return m_make_error(
            ErrorKind::Parse,
            "string_math: expected primary expression",
            m_position,
            m_position,
            {},
            {"primary expression"});
    }

    if (m_expression[m_position] == '(')
    {
        ++m_position;
        const auto inner_result = m_parse_expression(0);
        if (!inner_result)
        {
            return inner_result.error();
        }

        m_skip_spaces();
        if (m_position >= m_expression.size() || m_expression[m_position] != ')')
        {
            return m_make_error(
                ErrorKind::Parse,
                "string_math: expected ')'",
                m_position,
                m_position,
                {},
                {")"});
        }

        ++m_position;
        return inner_result.value();
    }

    if (m_expression[m_position] == '{')
    {
        const std::size_t open_brace_position = m_position;
        ++m_position;
        const auto variable_result = m_parse_variable_name(open_brace_position);
        if (!variable_result)
        {
            return variable_result.error();
        }

        const auto& variable = variable_result.value();
        m_variable_names.insert(variable.text);
        return m_add_node(detail::Node{
            detail::Node::Kind::Variable,
            {},
            variable.text,
            -1,
            -1,
            -1,
            0,
            {},
            variable.span,
        });
    }

    if (detail::is_identifier_start(m_expression[m_position]))
    {
        const ParsedText identifier = m_parse_identifier();
        m_skip_spaces();
        if (m_position < m_expression.size() && m_expression[m_position] == '(')
        {
            return m_parse_function_call(identifier);
        }

        if (const auto* entry = m_context.find_function(identifier.text); entry != nullptr && !entry->unary_overloads.empty())
        {
            const auto argument_result = m_parse_expression(Precedence::Prefix);
            if (!argument_result)
            {
                return argument_result.error();
            }

            return m_add_node(detail::Node{
                detail::Node::Kind::FunctionCall,
                {},
                identifier.text,
                argument_result.value(),
                -1,
                -1,
                1,
                {},
                identifier.span,
            });
        }

        return m_make_error(
            ErrorKind::NameResolution,
            "string_math: unknown identifier '" + identifier.text + "'",
            identifier.span,
            identifier.text);
    }

    if (m_looks_like_literal_start())
    {
        const ParsedText token = m_parse_literal_token();
        const auto literal_result = try_parse_literal(token.text, m_context, token.span);
        if (!literal_result)
        {
            return literal_result.error();
        }

        return m_add_node(detail::Node{
            detail::Node::Kind::Literal,
            literal_result.value(),
            {},
            -1,
            -1,
            -1,
            0,
            {},
            token.span,
        });
    }

    return m_make_error(
        ErrorKind::Parse,
        "string_math: expected primary expression",
        m_position,
        m_position + 1,
        std::string(1, m_expression[m_position]),
        {"primary expression"});
}

inline Result<int> detail::Parser::m_parse_function_call(ParsedText name)
{
    ++m_position;
    m_skip_spaces();
    std::vector<int> arguments;

    if (m_position < m_expression.size() && m_expression[m_position] != ')')
    {
        for (;;)
        {
            const auto argument_result = m_parse_expression(0);
            if (!argument_result)
            {
                return argument_result.error();
            }

            arguments.push_back(argument_result.value());
            m_skip_spaces();
            if (m_position >= m_expression.size() || m_expression[m_position] != ',')
            {
                break;
            }

            ++m_position;
            m_skip_spaces();
        }
    }

    if (m_position >= m_expression.size() || m_expression[m_position] != ')')
    {
        return m_make_error(
            ErrorKind::Parse,
            "string_math: expected ')' after function call",
            m_position,
            m_position,
            name.text,
            {")"});
    }

    ++m_position;
    return m_add_node(detail::Node{
        detail::Node::Kind::FunctionCall,
        {},
        std::move(name.text),
        arguments.empty() ? -1 : arguments.front(),
        arguments.size() > 1 ? arguments[1] : -1,
        -1,
        static_cast<int>(arguments.size()),
        std::move(arguments),
        name.span,
    });
}

inline detail::ParsedText detail::Parser::m_parse_identifier()
{
    const std::size_t begin = m_position++;
    while (m_position < m_expression.size() && detail::is_identifier_char(m_expression[m_position]))
    {
        ++m_position;
    }

    return ParsedText{
        std::string(m_expression.substr(begin, m_position - begin)),
        SourceSpan{begin, m_position},
    };
}

inline Result<detail::ParsedText> detail::Parser::m_parse_variable_name(std::size_t open_brace_position)
{
    const std::size_t begin = m_position;
    while (m_position < m_expression.size() && m_expression[m_position] != '}')
    {
        ++m_position;
    }

    if (m_position >= m_expression.size())
    {
        return m_make_error(
            ErrorKind::Parse,
            "string_math: expected '}'",
            open_brace_position,
            m_expression.size(),
            {},
            {"}"});
    }

    ParsedText text{
        std::string(detail::trim(m_expression.substr(begin, m_position - begin))),
        SourceSpan{open_brace_position, m_position + 1},
    };
    ++m_position;
    if (text.text.empty())
    {
        return m_make_error(
            ErrorKind::Parse,
            "string_math: empty variable name",
            text.span,
            {},
            {"variable name"});
    }

    return text;
}

inline bool detail::Parser::m_looks_like_literal_start() const noexcept
{
    const char ch = m_expression[m_position];
    return detail::is_digit(ch) || ch == '.';
}

inline detail::ParsedText detail::Parser::m_parse_literal_token()
{
    const std::size_t begin = m_position;
    while (m_position < m_expression.size())
    {
        const char ch = m_expression[m_position];
        if (detail::is_space(ch) || ch == ')' || ch == ',' || ch == '{' || ch == '}')
        {
            break;
        }

        const std::string infix = m_context.match_infix_symbol(m_expression.substr(m_position));
        const std::string postfix = m_context.match_postfix_symbol(m_expression.substr(m_position));
        if (m_position != begin && (!infix.empty() || !postfix.empty()))
        {
            const char previous = m_expression[m_position - 1];
            const bool signed_exponent = (ch == '+' || ch == '-') && (previous == 'e' || previous == 'E');
            const bool literal_alpha = detail::is_identifier_char(ch) || ch == '.';
            if (!signed_exponent && !literal_alpha)
            {
                break;
            }
        }

        ++m_position;
    }

    return ParsedText{
        std::string(m_expression.substr(begin, m_position - begin)),
        SourceSpan{begin, m_position},
    };
}

namespace detail
{

inline bool is_builtin_foldable_prefix(std::string_view symbol)
{
    return symbol == "+" || symbol == "-" || symbol == "!" || symbol == "abs" || symbol == "sqrt";
}

inline bool is_builtin_foldable_postfix(std::string_view symbol)
{
    return symbol == "!";
}

inline bool is_builtin_foldable_infix(std::string_view symbol)
{
    return symbol == "+" || symbol == "-" || symbol == "*" || symbol == "/" || symbol == "%" || symbol == "^" ||
        symbol == "log" || symbol == "max" || symbol == "min" || symbol == "==" || symbol == "!=" || symbol == "<" ||
        symbol == "<=" || symbol == ">" || symbol == ">=" || symbol == "&&" || symbol == "||";
}

inline bool is_builtin_foldable_function(std::string_view name)
{
    return name == "sin" || name == "cos" || name == "tan" || name == "asin" || name == "acos" || name == "atan" ||
        name == "sqrt" || name == "ceil" || name == "floor" || name == "round" || name == "exp" || name == "abs" ||
        name == "rad" || name == "deg" || name == "max" || name == "min";
}

inline Operation make_subtree_operation(const Operation& source, const Node& node, std::vector<Node> child_nodes)
{
    Operation operation;
    operation.m_nodes = std::move(child_nodes);
    operation.m_root = static_cast<int>(operation.m_nodes.size());
    operation.m_variables = source.m_variables;
    operation.m_nodes.push_back(node);
    return operation;
}

inline int fold_node(
    const Operation& source,
    const MathContext& context,
    int index,
    std::vector<Node>& optimized_nodes,
    std::unordered_map<int, int>& remapped_indices)
{
    const auto found = remapped_indices.find(index);
    if (found != remapped_indices.end())
    {
        return found->second;
    }

    Node node = source.m_nodes.at(static_cast<std::size_t>(index));
    if (node.left >= 0)
    {
        node.left = fold_node(source, context, node.left, optimized_nodes, remapped_indices);
    }
    if (node.right >= 0)
    {
        node.right = fold_node(source, context, node.right, optimized_nodes, remapped_indices);
    }
    if (node.third >= 0)
    {
        node.third = fold_node(source, context, node.third, optimized_nodes, remapped_indices);
    }
    for (int& argument : node.arguments)
    {
        argument = fold_node(source, context, argument, optimized_nodes, remapped_indices);
    }

    const auto child_is_literal = [&](int child_index) {
        return child_index >= 0 &&
            optimized_nodes.at(static_cast<std::size_t>(child_index)).kind == Node::Kind::Literal;
    };

    bool foldable = false;
    switch (node.kind)
    {
    case Node::Kind::PrefixOperator:
        foldable = child_is_literal(node.left) && is_builtin_foldable_prefix(node.text);
        break;
    case Node::Kind::PostfixOperator:
        foldable = child_is_literal(node.left) && is_builtin_foldable_postfix(node.text);
        break;
    case Node::Kind::InfixOperator:
        foldable = child_is_literal(node.left) && child_is_literal(node.right) && is_builtin_foldable_infix(node.text);
        break;
    case Node::Kind::Conditional:
        foldable = child_is_literal(node.left) && child_is_literal(node.right) && child_is_literal(node.third);
        break;
    case Node::Kind::FunctionCall:
        foldable = is_builtin_foldable_function(node.text);
        for (const int argument : node.arguments)
        {
            foldable = foldable && child_is_literal(argument);
        }
        break;
    default:
        break;
    }

    if (foldable)
    {
        Node rebuilt = node;
        std::vector<Node> child_nodes;
        if (rebuilt.left >= 0)
        {
            child_nodes.push_back(optimized_nodes.at(static_cast<std::size_t>(rebuilt.left)));
            rebuilt.left = 0;
        }
        if (rebuilt.right >= 0)
        {
            rebuilt.right = static_cast<int>(child_nodes.size());
            child_nodes.push_back(optimized_nodes.at(static_cast<std::size_t>(node.right)));
        }
        if (rebuilt.third >= 0)
        {
            rebuilt.third = static_cast<int>(child_nodes.size());
            child_nodes.push_back(optimized_nodes.at(static_cast<std::size_t>(node.third)));
        }
        if (!rebuilt.arguments.empty())
        {
            rebuilt.arguments.clear();
            for (const int argument : node.arguments)
            {
                rebuilt.arguments.push_back(static_cast<int>(child_nodes.size()));
                child_nodes.push_back(optimized_nodes.at(static_cast<std::size_t>(argument)));
            }
        }

        const auto literal_result = try_evaluate_operation(make_subtree_operation(source, rebuilt, std::move(child_nodes)), context);
        if (literal_result)
        {
            node.kind = Node::Kind::Literal;
            node.literal = literal_result.value();
            node.text.clear();
            node.left = -1;
            node.right = -1;
            node.third = -1;
            node.arity = 0;
            node.arguments.clear();
        }
    }

    const int optimized_index = static_cast<int>(optimized_nodes.size());
    optimized_nodes.push_back(std::move(node));
    remapped_indices.emplace(index, optimized_index);
    return optimized_index;
}

} // namespace detail

inline Result<Operation> try_optimize_operation(const BoundOperation& operation)
{
    Operation optimized;
    std::unordered_map<int, int> remapped_indices;
    optimized.m_root = detail::fold_node(
        operation.operation,
        operation.context.context(),
        operation.operation.m_root,
        optimized.m_nodes,
        remapped_indices);
    optimized.m_variables = operation.operation.m_variables;
    return optimized;
}

inline Result<Operation> try_create_operation(std::string_view expression, const MathContext& context)
{
    const auto parsed_result = try_parse_operation(expression, context);
    if (!parsed_result)
    {
        return parsed_result.error();
    }

    const auto bound_result = try_bind_operation(parsed_result.value(), context.freeze());
    if (!bound_result)
    {
        return bound_result.error();
    }

    return try_optimize_operation(bound_result.value());
}

inline Operation create_operation(std::string_view expression, const MathContext& context)
{
    return try_create_operation(expression, context).value();
}

inline Result<MathValue> try_evaluate_operation(const Operation& operation, const MathContext& context)
{
    const auto is_truthy = [](const MathValue& value) {
        return value.visit([](const auto& current) { return current != 0; });
    };

    std::function<Result<MathValue>(int)> evaluate_node = [&](int index) -> Result<MathValue> {
        const auto& node = operation.m_nodes.at(static_cast<std::size_t>(index));
        switch (node.kind)
        {
        case detail::Node::Kind::Literal:
            return node.literal;

        case detail::Node::Kind::Variable: {
            const auto value = context.find_value(node.text);
            if (!value)
            {
                return Error(
                    ErrorKind::NameResolution,
                    "string_math: unknown variable '" + node.text + "'",
                    node.span,
                    node.text);
            }
            return *value;
        }

        case detail::Node::Kind::PrefixOperator: {
            const auto operand_result = evaluate_node(node.left);
            if (!operand_result)
            {
                return operand_result.error();
            }

            const auto* entry = context.find_prefix_operator(node.text);
            if (entry == nullptr)
            {
                return Error(
                    ErrorKind::NameResolution,
                    "string_math: unknown prefix operator '" + node.text + "'",
                    node.span,
                    node.text);
            }

            const auto overload_result = detail::try_resolve_unary_overload(entry->overloads, operand_result.value().type(), node.text);
            if (!overload_result)
            {
                return Error(overload_result.error().kind(), overload_result.error().message(), node.span, node.text);
            }

            try
            {
                return overload_result.value()->invoke(overload_result.value()->storage.get(), operand_result.value());
            }
            catch (const std::exception& error)
            {
                return Error(ErrorKind::Evaluation, error.what(), node.span, node.text);
            }
        }

        case detail::Node::Kind::PostfixOperator: {
            const auto operand_result = evaluate_node(node.left);
            if (!operand_result)
            {
                return operand_result.error();
            }

            const auto* entry = context.find_postfix_operator(node.text);
            if (entry == nullptr)
            {
                return Error(
                    ErrorKind::NameResolution,
                    "string_math: unknown postfix operator '" + node.text + "'",
                    node.span,
                    node.text);
            }

            const auto overload_result = detail::try_resolve_unary_overload(entry->overloads, operand_result.value().type(), node.text);
            if (!overload_result)
            {
                return Error(overload_result.error().kind(), overload_result.error().message(), node.span, node.text);
            }

            try
            {
                return overload_result.value()->invoke(overload_result.value()->storage.get(), operand_result.value());
            }
            catch (const std::exception& error)
            {
                return Error(ErrorKind::Evaluation, error.what(), node.span, node.text);
            }
        }

        case detail::Node::Kind::InfixOperator: {
            const auto left_result = evaluate_node(node.left);
            if (!left_result)
            {
                return left_result.error();
            }

            const auto right_result = evaluate_node(node.right);
            if (!right_result)
            {
                return right_result.error();
            }

            const auto* entry = context.find_infix_operator(node.text);
            if (entry == nullptr)
            {
                return Error(
                    ErrorKind::NameResolution,
                    "string_math: unknown infix operator '" + node.text + "'",
                    node.span,
                    node.text);
            }

            const auto overload_result = detail::try_resolve_binary_overload(
                entry->overloads,
                left_result.value().type(),
                right_result.value().type(),
                node.text,
                context.policy().promotion);
            if (!overload_result)
            {
                return Error(overload_result.error().kind(), overload_result.error().message(), node.span, node.text);
            }

            try
            {
                return overload_result.value()
                    ->invoke(overload_result.value()->storage.get(), left_result.value(), right_result.value());
            }
            catch (const std::exception& error)
            {
                return Error(ErrorKind::Evaluation, error.what(), node.span, node.text);
            }
        }

        case detail::Node::Kind::FunctionCall: {
            const auto* entry = context.find_function(node.text);
            if (entry == nullptr)
            {
                return Error(
                    ErrorKind::NameResolution,
                    "string_math: unknown function '" + node.text + "'",
                    node.span,
                    node.text);
            }

            if (node.arity == 1)
            {
                const auto argument_result = evaluate_node(node.left);
                if (!argument_result)
                {
                    return argument_result.error();
                }

                const auto overload_result =
                    detail::try_resolve_unary_overload(entry->unary_overloads, argument_result.value().type(), node.text);
                if (!overload_result)
                {
                    return Error(overload_result.error().kind(), overload_result.error().message(), node.span, node.text);
                }

                try
                {
                    return overload_result.value()->invoke(overload_result.value()->storage.get(), argument_result.value());
                }
                catch (const std::exception& error)
                {
                    return Error(ErrorKind::Evaluation, error.what(), node.span, node.text);
                }
            }

            if (node.arity == 2)
            {
                const auto left_result = evaluate_node(node.left);
                if (!left_result)
                {
                    return left_result.error();
                }

                const auto right_result = evaluate_node(node.right);
                if (!right_result)
                {
                    return right_result.error();
                }

                const auto overload_result = detail::try_resolve_binary_overload(
                    entry->binary_overloads,
                    left_result.value().type(),
                    right_result.value().type(),
                    node.text,
                    context.policy().promotion);
                if (!overload_result)
                {
                    return Error(overload_result.error().kind(), overload_result.error().message(), node.span, node.text);
                }

                try
                {
                    return overload_result.value()
                        ->invoke(overload_result.value()->storage.get(), left_result.value(), right_result.value());
                }
                catch (const std::exception& error)
                {
                    return Error(ErrorKind::Evaluation, error.what(), node.span, node.text);
                }
            }

            if (!entry->variadic_overloads.empty())
            {
                std::vector<MathValue> arguments;
                arguments.reserve(node.arguments.size());
                for (const int argument_index : node.arguments)
                {
                    const auto argument_result = evaluate_node(argument_index);
                    if (!argument_result)
                    {
                        return argument_result.error();
                    }
                    arguments.push_back(argument_result.value());
                }

                for (const auto& overload : entry->variadic_overloads)
                {
                    if (arguments.size() < overload.min_arity)
                    {
                        continue;
                    }

                    try
                    {
                        return overload.invoke(overload.storage.get(), arguments);
                    }
                    catch (const std::exception& error)
                    {
                        return Error(ErrorKind::Evaluation, error.what(), node.span, node.text);
                    }
                }
            }

            return Error(ErrorKind::Evaluation, "string_math: unsupported function arity", node.span, node.text);
        }

        case detail::Node::Kind::Conditional: {
            const auto condition_result = evaluate_node(node.left);
            if (!condition_result)
            {
                return condition_result.error();
            }

            return is_truthy(condition_result.value()) ? evaluate_node(node.right) : evaluate_node(node.third);
        }
        }

        return Error(ErrorKind::Internal, "string_math: unreachable node kind");
    };

    if (operation.m_root < 0)
    {
        return Error(ErrorKind::Evaluation, "string_math: empty operation");
    }

    return evaluate_node(operation.m_root);
}

inline MathValue evaluate_operation(const Operation& operation, const MathContext& context)
{
    return try_evaluate_operation(operation, context).value();
}

} // namespace string_math
