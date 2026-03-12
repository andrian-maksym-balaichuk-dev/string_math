#include <iostream>
#include <string>

#include <string_math/string_math.hpp>

int main()
{
    string_math::Calculator calculator;
    auto& context = calculator.context();

    context.add_infix_operator(
        "avg", [](double left, double right) { return (left + right) / 2.0; }, string_math::Precedence::Additive,
        string_math::Associativity::Left);

    std::cout << "Enter expressions like 1+2, 1.0+2, 1f+3, {a}*4, 8 avg 4\n";
    calculator.set_value("a", 5);

    std::string expression;
    while (std::getline(std::cin, expression))
    {
        if (expression.empty())
        {
            continue;
        }

        try
        {
            const string_math::MathValue result = calculator.evaluate(expression);
            std::cout << result.to_string() << '\n';
        }
        catch (const std::exception& error)
        {
            std::cerr << "error: " << error.what() << '\n';
        }
    }

    return 0;
}
