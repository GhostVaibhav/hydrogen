#pragma once

#include <assert.h>

#include <unordered_map>

#include "parser.hpp"

class Generator {
 private:
  struct var {
    size_t stack_loc;
  };

  std::stringstream m_output;
  const NodeProg* m_prog;
  size_t m_stack_size = 0;
  std::unordered_map<std::string, var> m_vars{};

  void push(const std::string& reg) {
    m_output << "    push " << reg << "\n";
    ++m_stack_size;
  }

  void pop(const std::string& reg) {
    m_output << "    pop " << reg << "\n";
    --m_stack_size;
  }

 public:
  Generator(const NodeProg* prog) : m_prog(prog) {}

  void gen_term(const NodeTerm* term) {
    struct TermVisitor {
      Generator* gen;

      void operator()(const NodeTermIdent* term_ident) {
        if (!gen->m_vars.contains(term_ident->ident.value.value())) {
          std::cerr << "Undeclared identifier: "
                    << term_ident->ident.value.value() << std::endl;
          exit(EXIT_FAILURE);
        }
        const auto& var = gen->m_vars.at(term_ident->ident.value.value());
        gen->push("QWORD [rsp + " +
                  std::to_string((gen->m_stack_size - var.stack_loc - 1) * 8) +
                  "]");
      }

      void operator()(const NodeTermIntLit* term_int_lit) {
        gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value()
                      << "\n";
        gen->push("rax");
      }
    };

    TermVisitor visitor({.gen = this});
    std::visit(visitor, term->var);
  }

  void gen_expr(const NodeExpr* expr) {
    struct ExprVisitor {
      Generator* gen;

      void operator()(const NodeTerm* term) const { gen->gen_term(term); }
      void operator()(const NodeBinExpr* expr_bin) const {
        gen->gen_expr(expr_bin->add->lhs);
        gen->gen_expr(expr_bin->add->rhs);
        gen->pop("rax");
        gen->pop("rbx");
        gen->m_output << "    add rax, rbx\n";
        gen->push("rax");
      }
    };

    ExprVisitor visitor({.gen = this});
    std::visit(visitor, expr->var);
  }

  void gen_stmt(const NodeStmt* stmt) {
    struct StmtVisitor {
      Generator* gen;

      void operator()(const NodeStmtExit* stmt_exit) const {
        gen->gen_expr(stmt_exit->expr);

        gen->m_output << "    mov rax, 60\n";
        gen->pop("rdi");
        gen->m_output << "    syscall\n";
      }
      void operator()(const NodeStmtLet* stmt_let) const {
        if (gen->m_vars.contains(stmt_let->ident.value.value())) {
          std::cerr << "Duplicate identifiers ("
                    << stmt_let->ident.value.value() << ")" << std::endl;
          exit(EXIT_FAILURE);
        }
        gen->m_vars[stmt_let->ident.value.value()] =
            var{.stack_loc = gen->m_stack_size};
        gen->gen_expr(stmt_let->expr);
      }
    };

    StmtVisitor visitor({.gen = this});
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
