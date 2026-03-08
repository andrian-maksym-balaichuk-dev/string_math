#pragma once

#include <set>

#include <string_math/detail/literal.hpp>
#include <string_math/detail/overload.hpp>
#include <string_math/policy.hpp>
#include <string_math/result.hpp>
#include <string_math/value.hpp>

namespace string_math
{

class MathContext;
class FrozenMathContext;
class Operation;
class Calculator;
class MathExpr;
MathValue evaluate_operation(const Operation& operation, const MathContext& context);

struct UnaryOverloadInfo
{
    ValueType result_type{};
    ValueType argument_type{};
};

struct BinaryOverloadInfo
{
    ValueType result_type{};
    ValueType left_type{};
    ValueType right_type{};
};

struct FunctionInfo
{
    std::string name;
    std::vector<UnaryOverloadInfo> unary_overloads;
    std::vector<BinaryOverloadInfo> binary_overloads;
    std::vector<std::size_t> variadic_min_arities;
};

struct OperatorInfo
{
    std::string symbol;
    int precedence{0};
    Associativity associativity{Associativity::Left};
    std::vector<UnaryOverloadInfo> unary_overloads;
    std::vector<BinaryOverloadInfo> binary_overloads;
};

struct LiteralParserInfo
{
    std::string prefix;
    std::string suffix;
};

namespace detail
{

struct LiteralParserEntry
{
    std::string prefix;
    std::string suffix;
    fw::function_wrapper<std::optional<MathValue>(std::string_view)> parser;
};

struct PrefixOperatorEntry
{
    int precedence{ Precedence::Prefix };
    std::vector<UnaryOverload> overloads;
};

struct PostfixOperatorEntry
{
    int precedence{ Precedence::Postfix };
    std::vector<UnaryOverload> overloads;
};

struct InfixOperatorEntry
{
    int precedence{ Precedence::Additive };
    Associativity associativity{ Associativity::Left };
    std::vector<BinaryOverload> overloads;
};

struct FunctionEntry
{
    std::vector<UnaryOverload> unary_overloads;
    std::vector<BinaryOverload> binary_overloads;
    struct VariadicOverload
    {
        std::size_t min_arity{1};
        std::shared_ptr<const void> storage;
        MathValue (*invoke)(const void*, const std::vector<MathValue>&) = nullptr;
    };
    std::vector<VariadicOverload> variadic_overloads;
};

template <class T, class F>
struct unary_signature_for
{
    using result_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F&, T>>>;
    static_assert(is_supported_value_type_v<result_type>, "unsupported unary helper result type");
    using type = result_type(T);
};

template <class T, class F>
struct binary_signature_for
{
    using result_type = std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<F&, T, T>>>;
    static_assert(is_supported_value_type_v<result_type>, "unsupported binary helper result type");
    using type = result_type(T, T);
};

} // namespace detail

class MathContext
{
public:
    MathContext();
    MathContext(const MathContext& other);
    MathContext& operator=(const MathContext& other);
    MathContext(MathContext&&) noexcept = default;
    MathContext& operator=(MathContext&&) noexcept = default;
    explicit MathContext(const MathContext& parent, bool inherit_only);

    MathContext& set_parent(const MathContext& parent);
    MathContext with_parent(const MathContext& parent) const
    {
        MathContext copy(*this);
        copy.set_parent(parent);
        return copy;
    }
    MathContext& set_value(std::string name, MathValue value);
    MathContext with_value(std::string name, MathValue value) const
    {
        MathContext copy(*this);
        copy.set_value(std::move(name), value);
        return copy;
    }
    MathContext& add_variable(std::string name, MathValue value);
    MathContext& register_variable(std::string name, MathValue value)
    {
        return add_variable(std::move(name), value);
    }
    bool remove_variable(const std::string& name);
    std::optional<MathValue> find_value(std::string_view name) const;
    FrozenMathContext snapshot() const;
    FrozenMathContext freeze() const;
    std::vector<std::string> variable_names() const;
    MathContext& set_policy(EvaluationPolicy policy)
    {
        m_data->m_policy = policy;
        return *this;
    }
    MathContext with_policy(EvaluationPolicy policy) const
    {
        MathContext copy(*this);
        copy.set_policy(policy);
        return copy;
    }
    const EvaluationPolicy& policy() const noexcept
    {
        return m_data->m_policy;
    }

    template <class Sig, class F>
    MathContext& add_function(std::string name, F&& function)
    {
        using traits = detail::signature_traits<Sig>;
        auto& entry = m_data->m_functions[std::move(name)];
        if constexpr (traits::arity == 1)
        {
            detail::upsert_overload(entry.unary_overloads, detail::make_unary_overload<Sig>(std::forward<F>(function)));
        }
        else if constexpr (traits::arity == 2)
        {
            detail::upsert_overload(entry.binary_overloads, detail::make_binary_overload<Sig>(std::forward<F>(function)));
        }
        else
        {
            static_assert(detail::always_false_v<Sig>, "functions support arity 1 or 2 only");
        }
        return *this;
    }

    template <class F>
    MathContext& add_function(std::string name, F&& function)
    {
        return add_function<fw::detail::fn_sig_t<F>>(std::move(name), std::forward<F>(function));
    }

    template <class Sig, class F>
    MathContext with_function(std::string name, F&& function) const
    {
        MathContext copy(*this);
        copy.template add_function<Sig>(std::move(name), std::forward<F>(function));
        return copy;
    }

    template <class F>
    MathContext with_function(std::string name, F&& function) const
    {
        MathContext copy(*this);
        copy.add_function(std::move(name), std::forward<F>(function));
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_function(std::string name, F&& function)
    {
        return add_function<Sig>(std::move(name), std::forward<F>(function));
    }

    template <class F>
    MathContext& register_function(std::string name, F&& function)
    {
        return add_function(std::move(name), std::forward<F>(function));
    }

    template <class... Sigs, class F>
    MathContext& add_function_overloads(std::string name, F&& function)
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_function<Sigs>(name, shared_function), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_function_for(std::string name, F&& function)
    {
        return add_function_overloads<typename detail::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            std::move(name), std::forward<F>(function));
    }

    template <class... Ts, class F>
    MathContext& add_binary_function_for(std::string name, F&& function)
    {
        return add_function_overloads<typename detail::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            std::move(name), std::forward<F>(function));
    }

    template <class F>
    MathContext& add_variadic_function(std::string name, std::size_t min_arity, F&& function)
    {
        using wrapper_t = fw::function_wrapper<MathValue(const std::vector<MathValue>&)>;
        auto storage = std::make_shared<wrapper_t>(std::forward<F>(function));
        auto& entry = m_data->m_functions[std::move(name)];
        entry.variadic_overloads.push_back(typename detail::FunctionEntry::VariadicOverload{
            min_arity,
            storage,
            [](const void* raw_storage, const std::vector<MathValue>& arguments) {
                const auto& wrapper = *static_cast<const wrapper_t*>(raw_storage);
                return wrapper(arguments);
            },
        });
        return *this;
    }

    template <class T, class F>
    MathContext& add_variadic_function_for(std::string name, std::size_t min_arity, F&& function)
    {
        auto typed_function = std::decay_t<F>(std::forward<F>(function));
        return add_variadic_function(
            std::move(name),
            min_arity,
            [typed_function](const std::vector<MathValue>& arguments) mutable -> MathValue {
                std::vector<T> converted;
                converted.reserve(arguments.size());
                for (const auto& argument : arguments)
                {
                    converted.push_back(argument.template cast<T>());
                }
                return MathValue(typed_function(converted));
            });
    }

    template <class F>
    MathContext& register_variadic_function(std::string name, std::size_t min_arity, F&& function)
    {
        return add_variadic_function(std::move(name), min_arity, std::forward<F>(function));
    }

    template <class T, class F>
    MathContext& register_variadic_function_for(std::string name, std::size_t min_arity, F&& function)
    {
        return add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function));
    }

    template <class F>
    MathContext with_variadic_function(std::string name, std::size_t min_arity, F&& function) const
    {
        MathContext copy(*this);
        copy.add_variadic_function(std::move(name), min_arity, std::forward<F>(function));
        return copy;
    }

    template <class T, class F>
    MathContext with_variadic_function_for(std::string name, std::size_t min_arity, F&& function) const
    {
        MathContext copy(*this);
        copy.template add_variadic_function_for<T>(std::move(name), min_arity, std::forward<F>(function));
        return copy;
    }

    bool remove_function(const std::string& name);

    template <class Sig, class F>
    MathContext& add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        auto& entry = m_data->m_prefix_operators[std::move(symbol)];
        entry.precedence = precedence;
        detail::upsert_overload(entry.overloads, detail::make_unary_overload<Sig>(std::forward<F>(function)));
        m_rebuild_symbol_cache();
        return *this;
    }

    template <class F>
    MathContext& add_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator<fw::detail::fn_sig_t<F>>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class Sig, class F>
    MathContext with_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix) const
    {
        MathContext copy(*this);
        copy.template add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
        return copy;
    }

    template <class F>
    MathContext with_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix) const
    {
        MathContext copy(*this);
        copy.add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    MathContext& register_prefix_operator(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    MathContext& add_prefix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_prefix_operator<Sigs>(symbol, shared_function, precedence), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_prefix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Prefix)
    {
        return add_prefix_operator_overloads<typename detail::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            std::move(symbol), std::forward<F>(function), precedence);
    }

    bool remove_prefix_operator(const std::string& symbol);

    template <class Sig, class F>
    MathContext& add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        auto& entry = m_data->m_postfix_operators[std::move(symbol)];
        entry.precedence = precedence;
        detail::upsert_overload(entry.overloads, detail::make_unary_overload<Sig>(std::forward<F>(function)));
        m_rebuild_symbol_cache();
        return *this;
    }

    template <class F>
    MathContext& add_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator<fw::detail::fn_sig_t<F>>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class Sig, class F>
    MathContext with_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix) const
    {
        MathContext copy(*this);
        copy.template add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
        return copy;
    }

    template <class F>
    MathContext with_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix) const
    {
        MathContext copy(*this);
        copy.add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
        return copy;
    }

    template <class Sig, class F>
    MathContext& register_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class F>
    MathContext& register_postfix_operator(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator(std::move(symbol), std::forward<F>(function), precedence);
    }

    template <class... Sigs, class F>
    MathContext& add_postfix_operator_overloads(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_postfix_operator<Sigs>(symbol, shared_function, precedence), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext& add_postfix_operator_for(std::string symbol, F&& function, int precedence = Precedence::Postfix)
    {
        return add_postfix_operator_overloads<typename detail::unary_signature_for<Ts, std::decay_t<F>>::type...>(
            std::move(symbol), std::forward<F>(function), precedence);
    }

    bool remove_postfix_operator(const std::string& symbol);

    template <class Sig, class F>
    MathContext&
    add_infix_operator(std::string symbol, F&& function, int precedence, Associativity associativity = Associativity::Left)
    {
        auto& entry = m_data->m_infix_operators[std::move(symbol)];
        entry.precedence = precedence;
        entry.associativity = associativity;
        detail::upsert_overload(entry.overloads, detail::make_binary_overload<Sig>(std::forward<F>(function)));
        m_rebuild_symbol_cache();
        return *this;
    }

    template <class F>
    MathContext&
    add_infix_operator(std::string symbol, F&& function, int precedence, Associativity associativity = Associativity::Left)
    {
        return add_infix_operator<fw::detail::fn_sig_t<F>>(std::move(symbol), std::forward<F>(function), precedence, associativity);
    }

    template <class Sig, class F>
    MathContext with_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left) const
    {
        MathContext copy(*this);
        copy.template add_infix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence, associativity);
        return copy;
    }

    template <class F>
    MathContext with_infix_operator(
        std::string symbol,
        F&& function,
        int precedence,
        Associativity associativity = Associativity::Left) const
    {
        MathContext copy(*this);
        copy.add_infix_operator(std::move(symbol), std::forward<F>(function), precedence, associativity);
        return copy;
    }

    template <class Sig, class F>
    MathContext&
    register_infix_operator(std::string symbol, F&& function, int precedence, Associativity associativity = Associativity::Left)
    {
        return add_infix_operator<Sig>(std::move(symbol), std::forward<F>(function), precedence, associativity);
    }

    template <class F>
    MathContext&
    register_infix_operator(std::string symbol, F&& function, int precedence, Associativity associativity = Associativity::Left)
    {
        return add_infix_operator(std::move(symbol), std::forward<F>(function), precedence, associativity);
    }

    template <class... Sigs, class F>
    MathContext&
    add_infix_operator_overloads(std::string symbol, F&& function, int precedence, Associativity associativity = Associativity::Left)
    {
        auto shared_function = std::decay_t<F>(std::forward<F>(function));
        (add_infix_operator<Sigs>(symbol, shared_function, precedence, associativity), ...);
        return *this;
    }

    template <class... Ts, class F>
    MathContext&
    add_infix_operator_for(std::string symbol, F&& function, int precedence, Associativity associativity = Associativity::Left)
    {
        return add_infix_operator_overloads<typename detail::binary_signature_for<Ts, std::decay_t<F>>::type...>(
            std::move(symbol), std::forward<F>(function), precedence, associativity);
    }

    bool remove_infix_operator(const std::string& symbol);

    template <class F>
    MathContext& add_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        m_data->m_literal_parsers.push_back(
            detail::LiteralParserEntry{
                std::move(prefix),
                std::move(suffix),
                fw::function_wrapper<std::optional<MathValue>(std::string_view)>(std::forward<F>(parser)),
            });
        return *this;
    }

    template <class F>
    MathContext with_literal_parser(std::string prefix, std::string suffix, F&& parser) const
    {
        MathContext copy(*this);
        copy.add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
        return copy;
    }

    template <class F>
    MathContext& register_literal_parser(std::string prefix, std::string suffix, F&& parser)
    {
        return add_literal_parser(std::move(prefix), std::move(suffix), std::forward<F>(parser));
    }

    void clear_literal_parsers();
    std::optional<MathValue> try_parse_literal(std::string_view token) const;
    const detail::PrefixOperatorEntry* find_prefix_operator(std::string_view symbol) const;
    const detail::PostfixOperatorEntry* find_postfix_operator(std::string_view symbol) const;
    const detail::InfixOperatorEntry* find_infix_operator(std::string_view symbol) const;
    const detail::FunctionEntry* find_function(std::string_view name) const;
    std::string match_prefix_symbol(std::string_view text) const;
    std::string match_postfix_symbol(std::string_view text) const;
    std::string match_infix_symbol(std::string_view text) const;
    std::vector<std::string> function_names() const;
    std::vector<std::string> prefix_operator_names() const;
    std::vector<std::string> postfix_operator_names() const;
    std::vector<std::string> infix_operator_names() const;
    std::vector<LiteralParserInfo> literal_parsers() const;
    std::optional<FunctionInfo> inspect_function(std::string_view name) const;
    std::optional<OperatorInfo> inspect_prefix_operator(std::string_view symbol) const;
    std::optional<OperatorInfo> inspect_postfix_operator(std::string_view symbol) const;
    std::optional<OperatorInfo> inspect_infix_operator(std::string_view symbol) const;
    static MathContext with_builtins();

private:
    enum class SymbolKind
    {
        Prefix,
        Postfix,
        Infix
    };

    struct Data
    {
        std::shared_ptr<Data> m_parent;
        std::unordered_map<std::string, MathValue> m_variables;
        std::unordered_map<std::string, detail::FunctionEntry> m_functions;
        std::unordered_map<std::string, detail::PrefixOperatorEntry> m_prefix_operators;
        std::unordered_map<std::string, detail::PostfixOperatorEntry> m_postfix_operators;
        std::unordered_map<std::string, detail::InfixOperatorEntry> m_infix_operators;
        std::vector<detail::LiteralParserEntry> m_literal_parsers;
        std::vector<std::string> m_prefix_symbols;
        std::vector<std::string> m_postfix_symbols;
        std::vector<std::string> m_infix_symbols;
        EvaluationPolicy m_policy{};
    };

    std::shared_ptr<Data> m_data;

    std::string m_match_symbol(std::string_view text, SymbolKind kind) const;
    void m_rebuild_symbol_cache();
};

class FrozenMathContext
{
public:
    FrozenMathContext() = default;

    explicit FrozenMathContext(MathContext context) : m_context(std::move(context)) {}

    const MathContext& context() const noexcept
    {
        return m_context;
    }

    operator const MathContext&() const noexcept
    {
        return m_context;
    }

private:
    MathContext m_context;
};

inline MathContext::MathContext() : m_data(std::make_shared<Data>()) {}

inline MathContext::MathContext(const MathContext& other) : m_data(std::make_shared<Data>(*other.m_data)) {}

inline MathContext& MathContext::operator=(const MathContext& other)
{
    if (this != &other)
    {
        m_data = std::make_shared<Data>(*other.m_data);
    }
    return *this;
}

inline MathContext::MathContext(const MathContext& parent, bool inherit_only) : m_data(std::make_shared<Data>())
{
    (void)inherit_only;
    m_data->m_parent = parent.m_data;
}

inline MathContext& MathContext::set_parent(const MathContext& parent)
{
    m_data->m_parent = parent.m_data;
    return *this;
}

inline MathContext& MathContext::set_value(std::string name, MathValue value)
{
    m_data->m_variables[std::move(name)] = value;
    return *this;
}

inline MathContext& MathContext::add_variable(std::string name, MathValue value)
{
    return set_value(std::move(name), value);
}

inline bool MathContext::remove_variable(const std::string& name)
{
    return m_data->m_variables.erase(name) != 0;
}

inline std::optional<MathValue> MathContext::find_value(std::string_view name) const
{
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_variables.find(std::string(name));
        if (found != current->m_variables.end())
        {
            return found->second;
        }
    }
    return std::nullopt;
}

inline FrozenMathContext MathContext::snapshot() const
{
    return FrozenMathContext(*this);
}

inline FrozenMathContext MathContext::freeze() const
{
    return snapshot();
}

inline std::vector<std::string> MathContext::variable_names() const
{
    std::set<std::string> names;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_variables)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline bool MathContext::remove_function(const std::string& name)
{
    return m_data->m_functions.erase(name) != 0;
}

inline bool MathContext::remove_prefix_operator(const std::string& symbol)
{
    const bool removed = m_data->m_prefix_operators.erase(symbol) != 0;
    if (removed)
    {
        m_rebuild_symbol_cache();
    }
    return removed;
}

inline bool MathContext::remove_postfix_operator(const std::string& symbol)
{
    const bool removed = m_data->m_postfix_operators.erase(symbol) != 0;
    if (removed)
    {
        m_rebuild_symbol_cache();
    }
    return removed;
}

inline bool MathContext::remove_infix_operator(const std::string& symbol)
{
    const bool removed = m_data->m_infix_operators.erase(symbol) != 0;
    if (removed)
    {
        m_rebuild_symbol_cache();
    }
    return removed;
}

inline void MathContext::clear_literal_parsers()
{
    m_data->m_literal_parsers.clear();
}

inline std::optional<MathValue> MathContext::try_parse_literal(std::string_view token) const
{
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            if (!parser.prefix.empty() && !detail::starts_with(token, parser.prefix))
            {
                continue;
            }
            if (!parser.suffix.empty() && !detail::ends_with(token, parser.suffix))
            {
                continue;
            }
            if (auto value = parser.parser(token))
            {
                return value;
            }
        }
    }
    return std::nullopt;
}

inline const detail::PrefixOperatorEntry* MathContext::find_prefix_operator(std::string_view symbol) const
{
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_prefix_operators.find(std::string(symbol));
        if (found != current->m_prefix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const detail::PostfixOperatorEntry* MathContext::find_postfix_operator(std::string_view symbol) const
{
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_postfix_operators.find(std::string(symbol));
        if (found != current->m_postfix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const detail::InfixOperatorEntry* MathContext::find_infix_operator(std::string_view symbol) const
{
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_infix_operators.find(std::string(symbol));
        if (found != current->m_infix_operators.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline const detail::FunctionEntry* MathContext::find_function(std::string_view name) const
{
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto found = current->m_functions.find(std::string(name));
        if (found != current->m_functions.end())
        {
            return &found->second;
        }
    }
    return nullptr;
}

inline std::string MathContext::match_prefix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Prefix);
}

inline std::string MathContext::match_postfix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Postfix);
}

inline std::string MathContext::match_infix_symbol(std::string_view text) const
{
    return m_match_symbol(text, SymbolKind::Infix);
}

inline std::vector<std::string> MathContext::function_names() const
{
    std::set<std::string> names;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_functions)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<std::string> MathContext::prefix_operator_names() const
{
    std::set<std::string> names;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_prefix_operators)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<std::string> MathContext::postfix_operator_names() const
{
    std::set<std::string> names;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_postfix_operators)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<std::string> MathContext::infix_operator_names() const
{
    std::set<std::string> names;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& entry : current->m_infix_operators)
        {
            names.insert(entry.first);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

inline std::vector<LiteralParserInfo> MathContext::literal_parsers() const
{
    std::vector<LiteralParserInfo> parsers;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        for (const auto& parser : current->m_literal_parsers)
        {
            parsers.push_back(LiteralParserInfo{parser.prefix, parser.suffix});
        }
    }
    return parsers;
}

inline std::optional<FunctionInfo> MathContext::inspect_function(std::string_view name) const
{
    const auto* entry = find_function(name);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    FunctionInfo info;
    info.name = std::string(name);
    for (const auto& overload : entry->unary_overloads)
    {
        info.unary_overloads.push_back(UnaryOverloadInfo{overload.result_type, overload.argument_type});
    }
    for (const auto& overload : entry->binary_overloads)
    {
        info.binary_overloads.push_back(BinaryOverloadInfo{overload.result_type, overload.left_type, overload.right_type});
    }
    for (const auto& overload : entry->variadic_overloads)
    {
        info.variadic_min_arities.push_back(overload.min_arity);
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_prefix_operator(std::string_view symbol) const
{
    const auto* entry = find_prefix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    for (const auto& overload : entry->overloads)
    {
        info.unary_overloads.push_back(UnaryOverloadInfo{overload.result_type, overload.argument_type});
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_postfix_operator(std::string_view symbol) const
{
    const auto* entry = find_postfix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    for (const auto& overload : entry->overloads)
    {
        info.unary_overloads.push_back(UnaryOverloadInfo{overload.result_type, overload.argument_type});
    }
    return info;
}

inline std::optional<OperatorInfo> MathContext::inspect_infix_operator(std::string_view symbol) const
{
    const auto* entry = find_infix_operator(symbol);
    if (entry == nullptr)
    {
        return std::nullopt;
    }

    OperatorInfo info;
    info.symbol = std::string(symbol);
    info.precedence = entry->precedence;
    info.associativity = entry->associativity;
    for (const auto& overload : entry->overloads)
    {
        info.binary_overloads.push_back(BinaryOverloadInfo{overload.result_type, overload.left_type, overload.right_type});
    }
    return info;
}

inline std::string MathContext::m_match_symbol(std::string_view text, SymbolKind kind) const
{
    std::string best;
    for (const Data* current = m_data.get(); current != nullptr; current = current->m_parent.get())
    {
        const auto* symbols = &current->m_prefix_symbols;
        if (kind == SymbolKind::Postfix)
        {
            symbols = &current->m_postfix_symbols;
        }
        else if (kind == SymbolKind::Infix)
        {
            symbols = &current->m_infix_symbols;
        }

        for (const auto& symbol : *symbols)
        {
            if (text.size() >= symbol.size() && text.substr(0, symbol.size()) == symbol && symbol.size() > best.size())
            {
                best = symbol;
            }
        }
    }

    return best;
}

inline void MathContext::m_rebuild_symbol_cache()
{
    auto rebuild = [](const auto& map) {
        std::vector<std::string> symbols;
        symbols.reserve(map.size());
        for (const auto& entry : map)
        {
            symbols.push_back(entry.first);
        }
        std::sort(symbols.begin(), symbols.end(), [](const std::string& left, const std::string& right) {
            if (left.size() != right.size())
            {
                return left.size() > right.size();
            }
            return left < right;
        });
        return symbols;
    };

    m_data->m_prefix_symbols = rebuild(m_data->m_prefix_operators);
    m_data->m_postfix_symbols = rebuild(m_data->m_postfix_operators);
    m_data->m_infix_symbols = rebuild(m_data->m_infix_operators);
}

inline Result<MathValue> try_parse_literal(std::string_view token, const MathContext& context, SourceSpan span = {})
{
    try
    {
        if (auto custom = context.try_parse_literal(token))
        {
            return *custom;
        }
    }
    catch (const std::exception& error)
    {
        return Error(ErrorKind::Literal, error.what(), span, std::string(token));
    }

    try
    {
        return parse_builtin_literal(token);
    }
    catch (const std::exception& error)
    {
        return Error(ErrorKind::Literal, error.what(), span, std::string(token));
    }
}

inline MathValue parse_literal(std::string_view token, const MathContext& context)
{
    return try_parse_literal(token, context).value();
}

} // namespace string_math
