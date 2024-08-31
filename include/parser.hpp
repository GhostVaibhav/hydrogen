#pragma once

#include <variant>

#include "allocator.hpp"
#include "tokenizer.hpp"

struct NodeTermIntLit {
  Token int_lit;
};

struct NodeTermIdent {
  Token ident;
};

struct NodeExpr;

struct NodeTermParen {
  NodeExpr* expr;
};

struct NodeTerm {
  std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

struct NodeBinExprAdd {
  NodeExpr* lhs;
  NodeExpr* rhs;
};

struct NodeBinExprMul {
  NodeExpr* lhs;
  NodeExpr* rhs;
};

struct NodeBinExprSub {
  NodeExpr* lhs;
  NodeExpr* rhs;
};

struct NodeBinExprDiv {
  NodeExpr* lhs;
  NodeExpr* rhs;
};

struct NodeBinExpr {
  std::variant<NodeBinExprMul*, NodeBinExprAdd*, NodeBinExprDiv*,
               NodeBinExprSub*>
      var;
};

struct NodeExpr {
  std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit {
  NodeExpr* expr;
};

struct NodeStmtLet {
  Token ident;
  NodeExpr* expr;
};

struct NodeStmt;

struct NodeScope {
  std::vector<NodeStmt*> stmts;
};

struct NodeIfPred;

struct NodeIfPredElif {
  NodeExpr* expr;
  NodeScope* scope;
  std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
  NodeScope* scope;
};

struct NodeIfPred {
  std::variant<NodeIfPredElif*, NodeIfPredElse*> var;
};

struct NodeStmtIf {
  NodeExpr* expr;
  NodeScope* scope;
  std::optional<NodeIfPred*> pred;
};

struct NodeStmtReAssign {
  Token ident;
  NodeExpr* expr;
};

struct NodeStmt {
  std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*,
               NodeStmtReAssign*>
      var;
};

struct NodeProg {
  std::vector<NodeStmt*> stmts;
};

class Parser {
 private:
  std::optional<Token> peek(int offset = 0) const {
    if (m_index + offset >= m_tokens.size()) {
      return {};
    } else {
      return m_tokens.at(m_index + offset);
    }
  }

  Token consume() { return m_tokens.at(m_index++); }

  Token try_consume(TokenType type, const std::string& err_msg) {
    if (peek().has_value() && peek().value().type == type) {
      return consume();
    } else {
      std::cerr << err_msg << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  std::optional<Token> try_consume(TokenType type) {
    if (peek().has_value() && peek().value().type == type) {
      return consume();
    } else {
      return {};
    }
  }

  const std::vector<Token> m_tokens;
  size_t m_index = 0;
  ArenaAllocator m_allocator;

 public:
  explicit Parser(std::vector<Token> tokens)
      : m_tokens(std::move(tokens)), m_allocator(1024 * 1024 * 4) {}

  std::optional<NodeIfPred*> parse_if_pred() {
    if (try_consume(TokenType::_elif)) {
      try_consume(TokenType::_open_paren, "Expected open parentheses");
      auto elif = m_allocator.alloc<NodeIfPredElif>();
      if (auto expr = parse_expr()) {
        elif->expr = expr.value();
      } else {
        std::cerr << "Expression expected" << std::endl;
        exit(EXIT_FAILURE);
      }
      try_consume(TokenType::_close_paren, "Expected closed parentheses");
      if (auto scope = parse_scope()) {
        elif->scope = scope.value();
      } else {
        std::cerr << "Expected scope" << std::endl;
        exit(EXIT_FAILURE);
      }
      elif->pred = parse_if_pred();
      auto pred = m_allocator.alloc<NodeIfPred>();
      pred->var = elif;
      return pred;
    } else if (try_consume(TokenType::_else)) {
      auto else_ = m_allocator.alloc<NodeIfPredElse>();
      if (auto scope = parse_scope()) {
        else_->scope = scope.value();
      } else {
        std::cerr << "Expected scope" << std::endl;
        exit(EXIT_FAILURE);
      }
      auto pred = m_allocator.alloc<NodeIfPred>();
      pred->var = else_;
      return pred;
    }
    return {};
  }

  std::optional<NodeTerm*> parse_term() {
    if (auto int_lit = try_consume(TokenType::_int_lit)) {
      auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
      term_int_lit->int_lit = int_lit.value();
      auto term = m_allocator.alloc<NodeTerm>();
      term->var = term_int_lit;
      return term;
    } else if (auto ident = try_consume(TokenType::_ident)) {
      auto term_ident = m_allocator.alloc<NodeTermIdent>();
      term_ident->ident = ident.value();
      auto term = m_allocator.alloc<NodeTerm>();
      term->var = term_ident;
      return term;
    } else if (auto open_paren = try_consume(TokenType::_open_paren)) {
      auto term_paren = m_allocator.alloc<NodeTermParen>();
      auto expr = parse_expr();
      if (!expr.has_value()) {
        std::cerr << "Expected some expression" << std::endl;
        exit(EXIT_FAILURE);
      }
      try_consume(TokenType::_close_paren, "Expected close parenthesis");
      term_paren->expr = expr.value();
      auto term = m_allocator.alloc<NodeTerm>();
      term->var = term_paren;
      return term;
    } else {
      return {};
    }
  }

  std::optional<NodeExpr*> parse_expr(int min_prec = 0) {
    auto term_lhs = parse_term();

    if (!term_lhs.has_value()) {
      return {};
    }
    auto expr_lhs = m_allocator.alloc<NodeExpr>();
    expr_lhs->var = term_lhs.value();

    while (true) {
      auto curToken = peek();
      std::optional<int> prec;
      if (curToken.has_value()) {
        prec = bin_prec(curToken.value().type);
        if (!prec.has_value() || prec.value() < min_prec) {
          break;
        }
      } else {
        break;
      }
      auto op = consume();
      int next_min_prec = prec.value() + 1;
      auto expr_rhs = parse_expr(next_min_prec);

      if (!expr_rhs.has_value()) {
        std::cerr << "Unable to evaluate expression" << std::endl;
        exit(EXIT_FAILURE);
      }

      auto expr = m_allocator.alloc<NodeBinExpr>();
      auto expr_lhs_copy = m_allocator.alloc<NodeExpr>();

      if (op.type == TokenType::_op_add) {
        auto add = m_allocator.alloc<NodeBinExprAdd>();
        expr_lhs_copy->var = expr_lhs->var;
        add->lhs = expr_lhs_copy;
        add->rhs = expr_rhs.value();
        expr->var = add;
      } else if (op.type == TokenType::_op_mul) {
        auto mul = m_allocator.alloc<NodeBinExprMul>();
        expr_lhs_copy->var = expr_lhs->var;
        mul->lhs = expr_lhs_copy;
        mul->rhs = expr_rhs.value();
        expr->var = mul;
      } else if (op.type == TokenType::_op_sub) {
        auto sub = m_allocator.alloc<NodeBinExprSub>();
        expr_lhs_copy->var = expr_lhs->var;
        sub->lhs = expr_lhs_copy;
        sub->rhs = expr_rhs.value();
        expr->var = sub;
      } else if (op.type == TokenType::_op_div) {
        auto div = m_allocator.alloc<NodeBinExprDiv>();
        expr_lhs_copy->var = expr_lhs->var;
        div->lhs = expr_lhs_copy;
        div->rhs = expr_rhs.value();
        expr->var = div;
      } else {
        assert(false);
      }

      expr_lhs->var = expr;
    }

    return expr_lhs;
  }

  std::optional<NodeScope*> parse_scope() {
    if (!try_consume(TokenType::_open_braces).has_value()) {
      return {};
    }
    auto scope = m_allocator.alloc<NodeScope>();
    while (auto stmt = parse_stmt()) {
      scope->stmts.push_back(stmt.value());
    }
    try_consume(TokenType::_closed_braces, "Expected closed braces");
    return scope;
  }

  std::optional<NodeStmt*> parse_stmt() {
    if (peek().has_value() && peek().value().type == TokenType::_exit &&
        peek(1).has_value() && peek(1).value().type == TokenType::_open_paren) {
      consume();
      consume();
      auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
      if (auto nodeExpr = parse_expr()) {
        auto expr = m_allocator.alloc<NodeExpr>();
        expr = nodeExpr.value();
        stmt_exit->expr = expr;
      } else {
        std::cerr << "Unable to parse expression in exit node" << std::endl;
        exit(EXIT_FAILURE);
      }

      try_consume(TokenType::_close_paren, "Expected a closing parenthesis");
      try_consume(TokenType::_semi, "Did you forget the semicolon?");

      auto stmt = m_allocator.alloc<NodeStmt>();
      stmt->var = stmt_exit;
      return stmt;
    } else if (peek().has_value() && peek().value().type == TokenType::_let &&
               peek(1).has_value() &&
               peek(1).value().type == TokenType::_ident &&
               peek(2).has_value() &&
               peek(2).value().type == TokenType::_op_eq) {
      consume();
      auto stmt_let = m_allocator.alloc<NodeStmtLet>();
      stmt_let->ident = consume();
      consume();
      if (auto expr = parse_expr()) {
        stmt_let->expr = expr.value();
      } else {
        std::cerr << "Invalid expression" << std::endl;
        exit(EXIT_FAILURE);
      }
      try_consume(TokenType::_semi, "Did you forget the semicolon?");
      auto stmt = m_allocator.alloc<NodeStmt>();
      stmt->var = stmt_let;
      return stmt;
    } else if (peek().has_value() && peek().value().type == TokenType::_ident &&
               peek(1).has_value() &&
               peek(1).value().type == TokenType::_op_eq) {
      auto assign = m_allocator.alloc<NodeStmtReAssign>();
      assign->ident = consume();
      consume();
      if (auto expr = parse_expr()) {
        assign->expr = expr.value();
      } else {
        std::cerr << "Expected expression" << std::endl;
        exit(EXIT_FAILURE);
      }
      try_consume(TokenType::_semi, "Expected semi-colon");
      auto stmt = m_allocator.alloc<NodeStmt>();
      stmt->var = assign;
      return stmt;
    } else if (peek().has_value() &&
               peek().value().type == TokenType::_open_braces) {
      if (auto scope = parse_scope()) {
        auto stmt = m_allocator.alloc<NodeStmt>();
        stmt->var = scope.value();
        return stmt;
      } else {
        std::cerr << "Invalid scope\n";
        exit(EXIT_FAILURE);
      }
    } else if (auto if_ = try_consume(TokenType::_if)) {
      try_consume(TokenType::_open_paren, "Expected open parenthesis");
      auto stmt_if = m_allocator.alloc<NodeStmtIf>();
      if (auto expr = parse_expr()) {
        stmt_if->expr = expr.value();
      } else {
        std::cerr << "Invalid expresion inside if\n";
        exit(EXIT_FAILURE);
      }
      try_consume(TokenType::_close_paren, "Expected closed parenthesis");
      if (auto scope = parse_scope()) {
        stmt_if->scope = scope.value();
      } else {
        std::cerr << "Invalid scope\n";
        exit(EXIT_FAILURE);
      }
      stmt_if->pred = parse_if_pred();
      auto stmt = m_allocator.alloc<NodeStmt>();
      stmt->var = stmt_if;
      return stmt;
    }
    return {};
  }

  std::optional<NodeProg*> parse_prog() {
    auto prog = m_allocator.alloc<NodeProg>();
    while (peek().has_value()) {
      if (auto stmt = parse_stmt()) {
        prog->stmts.push_back(stmt.value());
      } else {
        std::cerr << "Invalid statement" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    return prog;
  }
};
