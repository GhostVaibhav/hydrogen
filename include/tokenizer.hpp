#pragma once

enum TokenType {
  _if,
  _exit,
  _int_lit,
  _semi,
  _open_paren,
  _close_paren,
  _open_braces,
  _closed_braces,
  _ident,
  _let,
  _op_eq,
  _op_add,
  _op_mul,
  _op_sub,
  _op_div
};

std::optional<int> bin_prec(TokenType type) {
  switch (type) {
    case TokenType::_op_add:
    case TokenType::_op_sub:
      return 0;
    case TokenType::_op_mul:
    case TokenType::_op_div:
      return 1;
    default:
      return {};
  }
}

struct Token {
  TokenType type;
  std::optional<std::string> value;
};

class Tokenizer {
 private:
  std::optional<char> peek(int offset = 0) const {
    if (m_index + offset >= m_src.size()) {
      return {};
    } else {
      return m_src.at(m_index + offset);
    }
  }

  char consume() { return m_src.at(m_index++); }

  const std::string m_src;
  size_t m_index = 0;

 public:
  explicit Tokenizer(std::string source) : m_src(std::move(source)) {}
  std::vector<Token> tokenize() {
    std::string buffer;
    std::vector<Token> tokens;

    while (peek().has_value()) {
      if (std::isalpha(peek().value())) {
        buffer.push_back(consume());
        while (peek().has_value() && std::isalnum(peek().value())) {
          buffer.push_back(consume());
        }

        if (buffer == "exit") {
          tokens.push_back({.type = TokenType::_exit});
          buffer.clear();
        } else if (buffer == "let") {
          tokens.push_back({.type = TokenType::_let});
          buffer.clear();
        } else if (buffer == "if") {
          tokens.push_back({.type = TokenType::_if});
          buffer.clear();
        } else {
          tokens.push_back({.type = TokenType::_ident, .value = buffer});
          buffer.clear();
        }
      } else if (std::isdigit(peek().value())) {
        buffer.push_back(consume());

        while (peek().has_value() && std::isdigit(peek().value())) {
          buffer.push_back(consume());
        }

        tokens.push_back({.type = TokenType::_int_lit, .value = buffer});
        buffer.clear();
      } else if (peek().value() == '(') {
        consume();
        tokens.push_back({.type = TokenType::_open_paren});
      } else if (peek().value() == ')') {
        consume();
        tokens.push_back({.type = TokenType::_close_paren});
      } else if (peek().value() == ';') {
        consume();
        tokens.push_back({.type = TokenType::_semi});
      } else if (peek().value() == '=') {
        consume();
        tokens.push_back({.type = TokenType::_op_eq});
      } else if (peek().value() == '+') {
        consume();
        tokens.push_back({.type = TokenType::_op_add});
      } else if (peek().value() == '*') {
        consume();
        tokens.push_back({.type = TokenType::_op_mul});
      } else if (peek().value() == '-') {
        consume();
        tokens.push_back({.type = TokenType::_op_sub});
      } else if (peek().value() == '/') {
        consume();
        tokens.push_back({.type = TokenType::_op_div});
      } else if (peek().value() == '{') {
        consume();
        tokens.push_back({.type = TokenType::_open_braces});
      } else if (peek().value() == '}') {
        consume();
        tokens.push_back({.type = TokenType::_closed_braces});
      } else if (std::isspace(peek().value())) {
        consume();
      } else {
        std::cerr << "Invalid syntax" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    m_index = 0;
    return tokens;
  }
};
