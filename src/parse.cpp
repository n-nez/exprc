#include <exprc/parse.h>

#include <istream>
#include <variant>
#include <string>
#include <memory>
#include <regex>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace exprc {

namespace ast {

namespace {

auto add(std::unique_ptr<Expr> a, std::unique_ptr<Expr> b) {
    return std::make_unique<Expr>(Add{std::move(a), std::move(b)});
}

auto mul(std::unique_ptr<Expr> a, std::unique_ptr<Expr> b) {
    return std::make_unique<Expr>(Mul{std::move(a), std::move(b)});
}

auto var(std::string name) {
    return std::make_unique<Expr>(Var{std::move(name)});
}

template <typename Type>
auto assign(std::string name, std::unique_ptr<Expr> e) {
    return Type{std::move(name), std::move(e)};
}

enum class Tok {
    VAR = 'v',
    OUT = 'o',
    ADD = '+',
    MUL = '*',
    SEM = ';',
    END = '\0',
    LPAREN = '(',
    RPAREN = ')',
    ASSIGN = '=',
};

struct Token {
    operator Tok() {
        return tok;
    }

    Tok tok;
    std::string value;
};

std::ostream& operator<<(std::ostream& os, const Token& token) {
    if (token.tok == Tok::VAR)
        return os << "<VAR<" << token.value << ">>";
    if (token.tok == Tok::OUT)
        return os << "<OUT>";
    return os << static_cast<char>(token.tok);
}

class Tokenizer {
public:
    Tokenizer(std::istream& is)
        : m_is(is)
        , m_iter(m_buf.cend())
        , m_eof(false) {
    }

    auto next() {
        if (m_eof)
            return Token{Tok::END};
        std::smatch match;
        while (true) {
            if (m_iter == m_buf.end()) {
                m_eof = !std::getline(m_is, m_buf);
                if (m_eof)
                    return Token{Tok::END};
                m_iter = m_buf.begin();
            }
            if (!std::regex_search(m_iter, m_buf.cend(), match, m_space))
                break;
            m_iter = match[0].second;
        }
        if (std::regex_search(m_iter, m_buf.cend(), match, m_out)) {
            m_iter = match[0].second;
            return Token{Tok::OUT};
        }
        if (std::regex_search(m_iter, m_buf.cend(), match, m_variable)) {
            m_iter = match[0].second;
            return Token{Tok::VAR, std::string(match[0].first, match[0].second)};
        }
        if (std::regex_search(m_iter, m_buf.cend(), match, m_symbol)) {
            m_iter = match[0].second;
            return Token{static_cast<Tok>(*match[0].first)};
        }
        throw std::invalid_argument(fmt::format("unexpected symbol {}", *m_iter));
    }

private:
    std::istream& m_is;
    std::string m_buf;
    std::string::const_iterator m_iter;
    bool m_eof;
    const std::regex m_space{R"(^\s+)"};
    const std::regex m_out{R"(^out\s+)"};
    const std::regex m_variable{R"(^[a-zA-Z0-9_]+)"};
    const std::regex m_symbol{R"(^[()+*=;])"};
};

class Parser {
public:
    Parser(std::istream& is)
        : m_tokenizer(is) {
    }

    // P -> { A ';' | 'out' A ';' }*
    std::list<Assign> parse() {
        std::list<Assign> program;
        next();
        while (m_tok != Tok::END) {
            if (m_tok == Tok::OUT) {
                next();
                program.emplace_back(parseAssign<AssignOut>());
            }
            else
                program.emplace_back(parseAssign<AssignVar>());
            if (m_tok != Tok::SEM)
                throw std::invalid_argument(fmt::format("unexpected trailing symbol {}", m_tok));
            next();
        }
        return program;
    }

private:
    void next() {
        m_tok = m_tokenizer.next();
    }

    // A -> V = E
    template <typename Type>
    Assign parseAssign() {
        if (m_tok != Tok::VAR)
            throw std::invalid_argument(fmt::format("expected varname given {}", m_tok));
        auto name = m_tok.value;
        next();
        if (m_tok != Tok::ASSIGN)
            throw std::invalid_argument(fmt::format("expected assignment given {}", m_tok));
        next();
        return Type{std::move(name), parseExpr()};
    }

    // E -> T { '+' }*
    std::unique_ptr<Expr> parseExpr() {
        auto expr = parseTerm();
        while (m_tok == Tok::ADD) {
            next();
            expr = add(std::move(expr), parseTerm());
        }
        return expr;
    }

    // T -> F { '*' }*
    std::unique_ptr<Expr> parseTerm() {
        auto term = parseFactor();
        while (m_tok == Tok::MUL) {
            next();
            term = mul(std::move(term), parseFactor());
        }
        return term;
    }

    // F -> V | '(' E ')'
    std::unique_ptr<Expr> parseFactor() {
        if (m_tok == Tok::VAR) {
            auto factor = var(m_tok.value);
            next();
            return factor;
        }
        if (m_tok == Tok::LPAREN) {
            next();
            auto factor = parseExpr();
            if (m_tok != Tok::RPAREN)
                throw std::invalid_argument(fmt::format("expected ')' given {}", m_tok));
            next();
            return factor;
        }
        throw std::invalid_argument(fmt::format("expected ')' or varname given {}", m_tok));
    }

    Tokenizer m_tokenizer;
    Token m_tok;
};

} // namespace

std::list<Assign> parse(std::istream& is) {
    return Parser(is).parse();
}

} // namespace ast

} // namespace exprc
