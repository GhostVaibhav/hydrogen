#pragma once

enum TokenType {
  _if,
  _elif,
  _else,
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

std::string token_to_string(const TokenType& type) {
  switch (type) {
    case _if:
      return "if";
    case _elif:
      return "elif";
    case _else:
      return "else";
    case _exit:
      return "exit";
    case _int_lit:
      return "int literal";
    case _semi:
      return ";";
    case _open_paren:
      return "(";
    case _close_paren:
      return ")";
    case _open_braces:
      return "{";
    case _closed_braces:
      return "}";
    case _ident:
      return "identifier";
    case _let:
      return "let";
    case _op_eq:
      return "=";
    case _op_add:
      return "+";
    case _op_mul:
      return "*";
    case _op_sub:
      return "-";
    case _op_div:
      return "/";
    default:
      return "";
  }
}

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
  int line;
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
    int line_count = 1;
    std::vector<Token> tokens;

    while (peek().has_value()) {
      if (std::isalpha(peek().value())) {
        buffer.push_back(consume());
        while (peek().has_value() && std::isalnum(peek().value())) {
          buffer.push_back(consume());
        }

        if (buffer == "exit") {
          tokens.push_back({.type = TokenType::_exit, .line = line_count});
          buffer.clear();
        } else if (buffer == "let") {
          tokens.push_back({.type = TokenType::_let, .line = line_count});
          buffer.clear();
        } else if (buffer == "if") {
          tokens.push_back({.type = TokenType::_if, .line = line_count});
          buffer.clear();
        } else if (buffer == "elif") {
          tokens.push_back({.type = TokenType::_elif, .line = line_count});
          buffer.clear();
        } else if (buffer == "else") {
          tokens.push_back({.type = TokenType::_else, .line = line_count});
          buffer.clear();
        } else {
          tokens.push_back(
              {.type = TokenType::_ident, .line = line_count, .value = buffer});
          buffer.clear();
        }
      } else if (std::isdigit(peek().value())) {
        buffer.push_back(consume());

        while (peek().has_value() && std::isdigit(peek().value())) {
          buffer.push_back(consume());
        }

        tokens.push_back(
            {.type = TokenType::_int_lit, .line = line_count, .value = buffer});
        buffer.clear();
      } else if (peek().value() == '/' && peek(1).has_value() &&
                 peek(1).value() == '/') {
        while (peek().has_value() && peek().value() != '\n') {
          consume();
        }
      } else if (peek().value() == '/' && peek(1).has_value() &&
                 peek(1).value() == '*') {
        consume();
        consume();
        while (peek().has_value()) {
          if (peek().value() == '*' && peek(1).has_value() &&
              peek(1).value() == '/')
            break;
          if (peek().has_value() && peek().value() == '\n') ++line_count;
          consume();
        }
        if (peek().has_value()) consume();
        if (peek().has_value()) consume();
      } else if (peek().value() == '(') {
        consume();
        tokens.push_back({.type = TokenType::_open_paren, .line = line_count});
      } else if (peek().value() == ')') {
        consume();
        tokens.push_back({.type = TokenType::_close_paren, .line = line_count});
      } else if (peek().value() == ';') {
        consume();
        tokens.push_back({.type = TokenType::_semi, .line = line_count});
      } else if (peek().value() == '=') {
        consume();
        tokens.push_back({.type = TokenType::_op_eq, .line = line_count});
      } else if (peek().value() == '+') {
        consume();
        tokens.push_back({.type = TokenType::_op_add, .line = line_count});
      } else if (peek().value() == '*') {
        consume();
        tokens.push_back({.type = TokenType::_op_mul, .line = line_count});
      } else if (peek().value() == '-') {
        consume();
        tokens.push_back({.type = TokenType::_op_sub, .line = line_count});
      } else if (peek().value() == '/') {
        consume();
        tokens.push_back({.type = TokenType::_op_div, .line = line_count});
      } else if (peek().value() == '{') {
        consume();
        tokens.push_back({.type = TokenType::_open_braces, .line = line_count});
      } else if (peek().value() == '}') {
        consume();
        tokens.push_back(
            {.type = TokenType::_closed_braces, .line = line_count});
      } else if (peek().value() == '\n') {
        consume();
        ++line_count;
      } else if (std::isspace(peek().value())) {
        consume();
      } else {
        std::cerr << "Invalid token" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    m_index = 0;
    return tokens;
  }
};
