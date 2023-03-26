#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <iostream>

enum class TokenType {
    Number,
    Operator,
    Symbol
};

class Token {
public:
    Token(const std::string& value) : d_value(value) {}
    virtual ~Token() = default;

    virtual TokenType type() const = 0;

    virtual std::string toString() const {
        std::string prefix = "('";
        std::string suffix = "': '" + d_value + "')";

        switch (type()) {
        case TokenType::Number:
            prefix += "Number";
            break;
        case TokenType::Operator:
            prefix += "Operator";
            break;
        case TokenType::Symbol:
            prefix += "Symbol";
            break;
        default:
            prefix += "Unknown";
            break;
        }

        return prefix + suffix;
    }

    std::string d_value;
};

class NumberToken : public Token {
public:
    NumberToken(const std::string& value) : Token(value) {}

    // Inherited via Token
    virtual TokenType type() const override { return TokenType::Number; }

    int64_t getIntValue() {
        return std::atoll(d_value.c_str());
    }
};

class OperatorToken : public Token {
public:
    OperatorToken(const std::string& value, uint32_t precedence = 1, bool leftAssociative = true, bool unary = false)
        : Token(value), d_precedence(precedence), d_leftAssociative(leftAssociative), d_unary(unary) {}

    uint32_t    d_precedence;
    bool        d_leftAssociative;
    bool        d_unary;

    // Inherited via Token
    virtual TokenType type() const override { return TokenType::Operator; }

    virtual std::string toString() const {
        return "('Operator': '" + d_value + "', " + (d_unary ? "unary" : "binary") + ")";
    }
};

class SymbolToken : public Token {
public:
    SymbolToken(const std::string& value) : Token(value) {}

    // Inherited via Token
    virtual TokenType type() const override { return TokenType::Symbol; }
};

using TokenRef = std::shared_ptr<Token>;

template <typename T, typename... Args>
constexpr std::shared_ptr<Token> makeToken(Args&& ... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
constexpr std::shared_ptr<T> as(Args&&... args)
{
    return std::static_pointer_cast<T>(std::forward<Args>(args)...);
}

/*
    Test Expression: 4 + 2 * (3 - 1)

    W/o  order-awareness : 4 + 2 * 3 - 1 = 6 * 3 - 1 = 18 - 1 = __17__ (WRONG!)
    With order-awareness : 4 + 2 * (2) = 4 + 4 = __8__ (CORRECT!)
*/

//static std::vector<TokenRef> TOKENS = {
//    makeToken<NumberToken>("4"),
//    makeToken<OperatorToken>("+", 1, true),
//    makeToken<NumberToken>("2"),
//    makeToken<OperatorToken>("*", 2, true),
//    makeToken<SymbolToken>("("),
//    makeToken<NumberToken>("3"),
//    makeToken<OperatorToken>("-", 1, true),
//    makeToken<NumberToken>("1"),
//    makeToken<SymbolToken>(")")
//};

//static std::vector<TokenRef> TOKENS = {
//    makeToken<NumberToken>("4"),
//    makeToken<OperatorToken>("-", 1, false),
//    makeToken<OperatorToken>("!", 2, true, true), // right-associative and unary
//    makeToken<NumberToken>("1"),
//};

// -6 + 2 * (-3 - 1) = -6 + (2 * -4) = -6 - 8 = -14 
static std::vector<TokenRef> TOKENS = {
    makeToken<OperatorToken>("-", 1, false),
    makeToken<NumberToken>("6"),
    makeToken<OperatorToken>("+", 1, true),
    makeToken<NumberToken>("2"),
    makeToken<OperatorToken>("*", 2, true),
    makeToken<SymbolToken>("("),
    makeToken<OperatorToken>("-", 1, false),
    makeToken<NumberToken>("3"),
    makeToken<OperatorToken>("-", 1, true),
    makeToken<NumberToken>("1"),
    makeToken<SymbolToken>(")")
};

TokenRef readToken(std::vector<TokenRef>& tokens) {
    auto token = tokens.at(0);
    tokens.erase(tokens.begin());

    return token;
}

std::stack<TokenRef> shuntingYardAlgorithm(std::vector<TokenRef>& inputQueue) {
    std::stack<TokenRef> outputStack;
    std::stack<TokenRef> operatorStack;

    TokenRef previousToken = nullptr;

    while (!inputQueue.empty()) {
        // Read the next token from the input queue
        auto token = readToken(inputQueue);

        // If the token is a number, we directly push ity to the output stack
        if (token->type() == TokenType::Number)
            outputStack.push(token);

        // If the token is a left parenthesis, it goes directly to the operator stack
        else if (token->d_value == "(")
            operatorStack.push(token);

        // Check if the token is an operator
        else if (token->type() == TokenType::Operator) {
            // Special check for a unary +/- operator
            if (token->d_value == "+" || token->d_value == "-") {
                if (!previousToken || (previousToken->type() == TokenType::Operator || previousToken->type() == TokenType::Symbol)) {
                    as<OperatorToken>(token)->d_unary = true;
                    as<OperatorToken>(token)->d_leftAssociative = false;
                }
            }

            while (!operatorStack.empty()) {
                if (operatorStack.top()->d_value == "(")
                    break;

                auto topOperator = as<OperatorToken>(operatorStack.top());
                auto currentOperator = as<OperatorToken>(token);

                if (topOperator->d_precedence < currentOperator->d_precedence)
                    break;
                else if ((topOperator->d_precedence == currentOperator->d_precedence) && !currentOperator->d_leftAssociative)
                    break;

                outputStack.push(operatorStack.top());
                operatorStack.pop();
            }

            // Push the current operator to the operator stack
            operatorStack.push(token);
        }

        // Check if the token is a closing (right) parenthesis
        else if (token->d_value == ")") {
            while (!operatorStack.empty()) {
                if (operatorStack.top()->d_value == "(")
                    break;

                outputStack.push(operatorStack.top());
                operatorStack.pop();
            }

            if (operatorStack.empty() || operatorStack.top()->d_value != "(") {
                std::cout << "Mismatched parenthesis error!\n";
                break;
            }

            // Pop the left parenthesis off the operator stack
            operatorStack.pop();
        }

        // Keep track of the previous token
        previousToken = token;
    }

    // Pop the remaining operators from the operator stack into the output stack
    while (!operatorStack.empty()) {
        outputStack.push(operatorStack.top());
        operatorStack.pop();
    }

    return outputStack;
}

int64_t evaluateExpressionTokens(std::stack<TokenRef>& expressionStack) {
    auto token = expressionStack.top();
    expressionStack.pop();

    if (token->type() == TokenType::Number)
        return as<NumberToken>(token)->getIntValue();

    if (token->type() == TokenType::Operator) {
        if (as<OperatorToken>(token)->d_unary) {
            int64_t rhs = evaluateExpressionTokens(expressionStack);
            if (token->d_value == "!")
                return static_cast<int64_t>(!static_cast<bool>(rhs));
            else if (token->d_value == "+")
                return rhs;
            else if (token->d_value == "-")
                return -rhs;
        } else {
            int64_t rhs = evaluateExpressionTokens(expressionStack);
            int64_t lhs = evaluateExpressionTokens(expressionStack);

            if (token->d_value == "+")
                return lhs + rhs;
            else if (token->d_value == "-")
                return lhs - rhs;
            else if (token->d_value == "*")
                return lhs * rhs;
            else if (token->d_value == "/")
                return lhs / rhs;
        }
    }

    std::cout << "Error evaluating token: " << token->toString() << "\n";
    return 0;
}

void printOutputExpressionStack(std::stack<TokenRef> expressionStack) {
    std::cout << "----- Expression Output Stack -----\n";
    while (!expressionStack.empty()) {
        std::cout << expressionStack.top()->toString() << "\n";
        expressionStack.pop();
    }
    std::cout << "\n";
}

int main() {
    for (auto& token : TOKENS) {
        std::cout << token->toString() << "\n";
    }
    std::cout << "\n";

    auto expressionStack = shuntingYardAlgorithm(TOKENS);
    printOutputExpressionStack(expressionStack);

    auto expressionResult = evaluateExpressionTokens(expressionStack);
    std::cout << "Expressiong result: " << expressionResult << "\n";

    return 0;
}
