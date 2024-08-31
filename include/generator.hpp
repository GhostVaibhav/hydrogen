#pragma once

#include <assert.h>

#include "parser.hpp"

class Generator {
 private:
  struct var {
    std::string name;
    size_t stack_loc;
  };

  std::stringstream m_output;
  const NodeProg* m_prog;
  size_t m_stack_size = 0;
  std::vector<var> m_vars{};
  std::vector<size_t> m_scopes{};
  int m_label_count = 0;

  void push(const std::string& reg) {
    m_output << "    push " << reg << "\n";
    ++m_stack_size;
  }

  void pop(const std::string& reg) {
    m_output << "    pop " << reg << "\n";
    --m_stack_size;
  }

  void begin_scope() { m_scopes.push_back(m_vars.size()); }
  void end_scope() {
    auto pop_count = m_vars.size() - m_scopes.back();
    m_output << "    add rsp, " << pop_count * 8 << std::endl;
    m_stack_size -= pop_count;
    for (int i = 0; i < pop_count; ++i) {
      m_vars.pop_back();
    }
    m_scopes.pop_back();
  }

  std::string create_label() {
    return "label" + std::to_string(m_label_count++);
  }

 public:
  Generator(const NodeProg* prog) : m_prog(prog) {}

  void gen_term(const NodeTerm* term) {
    struct TermVisitor {
      Generator& gen;

      void operator()(const NodeTermIdent* term_ident) {
        auto it = std::find_if(
            gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const var& _var) {
              return _var.name == term_ident->ident.value.value();
            });
        if (it == gen.m_vars.cend()) {
          std::cerr << "Undeclared identifier: "
                    << term_ident->ident.value.value() << std::endl;
          exit(EXIT_FAILURE);
        }
        gen.push("QWORD [rsp + " +
                 std::to_string((gen.m_stack_size - (*it).stack_loc - 1) * 8) +
                 "]");
      }

      void operator()(const NodeTermIntLit* term_int_lit) {
        gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value()
                     << "\n";
        gen.push("rax");
      }

      void operator()(const NodeTermParen* term_paren) const {
        gen.gen_expr(term_paren->expr);
      }
    };

    TermVisitor visitor({.gen = *this});
    std::visit(visitor, term->var);
  }

  void gen_bin_expr(const NodeBinExpr* bin_expr) {
    struct BinExprVisitor {
      Generator& gen;

      void operator()(const NodeBinExprAdd* add) const {
        gen.gen_expr(add->rhs);
        gen.gen_expr(add->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    add rax, rbx\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprMul* mul) const {
        gen.gen_expr(mul->rhs);
        gen.gen_expr(mul->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    mul rbx\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprSub* sub) const {
        gen.gen_expr(sub->rhs);
        gen.gen_expr(sub->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    sub rax, rbx\n";
        gen.push("rax");
      }
      void operator()(const NodeBinExprDiv* div) const {
        gen.gen_expr(div->rhs);
        gen.gen_expr(div->lhs);
        gen.pop("rax");
        gen.pop("rbx");
        gen.m_output << "    div rbx\n";
        gen.push("rax");
      }
    };

    BinExprVisitor visitor({.gen = *this});
    std::visit(visitor, bin_expr->var);
  }

  void gen_expr(const NodeExpr* expr) {
    struct ExprVisitor {
      Generator& gen;

      void operator()(const NodeTerm* term) const { gen.gen_term(term); }
      void operator()(const NodeBinExpr* expr_bin) const {
        gen.gen_bin_expr(expr_bin);
      }
    };

    ExprVisitor visitor({.gen = *this});
    std::visit(visitor, expr->var);
  }

  void gen_scope(const NodeScope* scope) {
    begin_scope();

    for (const auto stmt : scope->stmts) {
      gen_stmt(stmt);
    }

    end_scope();
  }

  void gen_if_pred(const NodeIfPred* pred, const std::string& end_label) {
    struct PredVisitor {
      Generator& gen;
      const std::string& end_label;

      void operator()(const NodeIfPredElif* _elif) const {
        gen.gen_expr(_elif->expr);
        gen.pop("rax");
        const std::string label = gen.create_label();
        gen.m_output << "    test rax, rax\n";
        gen.m_output << "    jz " << label << "\n";
        gen.gen_scope(_elif->scope);
        gen.m_output << "    jmp " << end_label << "\n";
        if (_elif->pred.has_value()) {
          gen.m_output << label << ":\n";
          gen.gen_if_pred(_elif->pred.value(), end_label);
        }
      }
      void operator()(const NodeIfPredElse* _else) const {
        gen.gen_scope(_else->scope);
      }
    };

    PredVisitor visitor({.gen = *this, .end_label = end_label});
    std::visit(visitor, pred->var);
  }

  void gen_stmt(const NodeStmt* stmt) {
    struct StmtVisitor {
      Generator& gen;

      void operator()(const NodeStmtExit* stmt_exit) const {
        gen.gen_expr(stmt_exit->expr);

        gen.m_output << "    mov rax, 60\n";
        gen.pop("rdi");
        gen.m_output << "    syscall\n";
      }

      void operator()(const NodeStmtLet* stmt_let) const {
        auto it = std::find_if(
            gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const var& _var) {
              return _var.name == stmt_let->ident.value.value();
            });
        if (it != gen.m_vars.cend()) {
          std::cerr << "Duplicate identifiers ("
                    << stmt_let->ident.value.value() << ")" << std::endl;
          exit(EXIT_FAILURE);
        }
        gen.m_vars.push_back({.name = stmt_let->ident.value.value(),
                              .stack_loc = gen.m_stack_size});
        gen.gen_expr(stmt_let->expr);
      }

      void operator()(const NodeScope* stmt_scope) const {
        gen.gen_scope(stmt_scope);
      }
      void operator()(const NodeStmtIf* stmt_if) const {
        gen.gen_expr(stmt_if->expr);
        gen.pop("rax");
        std::string label = gen.create_label();
        gen.m_output << "    test rax, rax\n";
        gen.m_output << "    jz " << label << "\n";
        gen.gen_scope(stmt_if->scope);
        std::optional<std::string> end_label;
        if (stmt_if->pred.has_value()) {
          end_label = gen.create_label();
          gen.m_output << "    jmp " << end_label.value() << "\n";
        }
        gen.m_output << label << ":\n";
        if (stmt_if->pred.has_value()) {
          gen.gen_if_pred(stmt_if->pred.value(), end_label.value());
          gen.m_output << end_label.value() << ":\n";
        }
      }
      void operator()(const NodeStmtReAssign* assign) const {
        auto it = std::find_if(gen.m_vars.begin(), gen.m_vars.end(),
                               [&](const var& elt) {
                                 return elt.name == assign->ident.value.value();
                               });
        if (it == gen.m_vars.end()) {
          std::cerr << "Undeclared identifier: " << assign->ident.value.value()
                    << std::endl;
          exit(EXIT_FAILURE);
        }
        gen.gen_expr(assign->expr);
        gen.pop("rax");
        gen.m_output << "    mov [rsp + "
                     << (gen.m_stack_size - it->stack_loc - 1) * 8
                     << "], rax\n";
      }
    };

    StmtVisitor visitor({.gen = *this});
    std::visit(visitor, stmt->var);
  }

  std::string gen_prog() {
    m_output << "global _start\n_start:\n";

    for (const NodeStmt* stmt : m_prog->stmts) {
      gen_stmt(stmt);
    }

    m_output << "    mov rax, 60\n";
    m_output << "    mov rdi, 0\n";
    m_output << "    syscall";

    return m_output.str();
  }
};
