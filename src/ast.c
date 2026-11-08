#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Symbol Symbol;
struct Symbol {
    char *name;
    int value;
    Symbol *next;
};

typedef struct {
    Symbol *symbols;
    int failed;
    int returned;
    int return_value;
} Runtime;

static Ast *new_ast(AstKind kind) {
    Ast *node = calloc(1, sizeof(*node));
    if (!node) {
        fputs("fatal: out of memory\n", stderr);
        exit(2);
    }
    node->kind = kind;
    return node;
}

Ast *ast_integer(int value) {
    Ast *node = new_ast(AST_INT);
    node->as.integer = value;
    return node;
}

Ast *ast_variable(char *name) {
    Ast *node = new_ast(AST_VAR);
    node->as.name = name;
    return node;
}

Ast *ast_binary(AstOp op, Ast *left, Ast *right) {
    Ast *node = new_ast(AST_BINARY);
    node->as.operation.op = op;
    node->as.operation.left = left;
    node->as.operation.right = right;
    return node;
}

Ast *ast_unary(AstOp op, Ast *operand) {
    Ast *node = new_ast(AST_UNARY);
    node->as.operation.op = op;
    node->as.operation.left = operand;
    return node;
}

Ast *ast_binding(AstKind kind, char *name, Ast *value) {
    Ast *node = new_ast(kind);
    node->as.binding.name = name;
    node->as.binding.value = value;
    return node;
}

Ast *ast_expression_statement(AstKind kind, Ast *expression) {
    Ast *node = new_ast(kind);
    node->as.expression = expression;
    return node;
}

Ast *ast_print_string(char *text) {
    Ast *node = new_ast(AST_PRINT_STRING);
    node->as.string = text;
    return node;
}

Ast *ast_if(Ast *condition, Ast *then_branch, Ast *else_branch) {
    Ast *node = new_ast(AST_IF);
    node->as.conditional.condition = condition;
    node->as.conditional.then_branch = then_branch;
    node->as.conditional.else_branch = else_branch;
    return node;
}

Ast *ast_while(Ast *condition, Ast *body) {
    Ast *node = new_ast(AST_WHILE);
    node->as.loop.condition = condition;
    node->as.loop.body = body;
    return node;
}

Ast *ast_append(Ast *list, Ast *node) {
    Ast *tail;
    if (!list) return node;
    for (tail = list; tail->next; tail = tail->next) {}
    tail->next = node;
    return list;
}

void ast_free(Ast *node) {
    Ast *next;
    while (node) {
        next = node->next;
        switch (node->kind) {
            case AST_VAR:
                free(node->as.name);
                break;
            case AST_BINARY:
                ast_free(node->as.operation.left);
                ast_free(node->as.operation.right);
                break;
            case AST_UNARY:
                ast_free(node->as.operation.left);
                break;
            case AST_DECL:
            case AST_ASSIGN:
                free(node->as.binding.name);
                ast_free(node->as.binding.value);
                break;
            case AST_PRINT_EXPR:
            case AST_RETURN:
                ast_free(node->as.expression);
                break;
            case AST_PRINT_STRING:
                free(node->as.string);
                break;
            case AST_IF:
                ast_free(node->as.conditional.condition);
                ast_free(node->as.conditional.then_branch);
                ast_free(node->as.conditional.else_branch);
                break;
            case AST_WHILE:
                ast_free(node->as.loop.condition);
                ast_free(node->as.loop.body);
                break;
            case AST_INT:
                break;
        }
        free(node);
        node = next;
    }
}

static Symbol *find_symbol(Runtime *runtime, const char *name) {
    Symbol *symbol;
    for (symbol = runtime->symbols; symbol; symbol = symbol->next) {
        if (strcmp(symbol->name, name) == 0) return symbol;
    }
    return NULL;
}

static int runtime_error(Runtime *runtime, const char *message, const char *name) {
    fprintf(stderr, "runtime error: %s%s%s\n", message, name ? " " : "", name ? name : "");
    runtime->failed = 1;
    return 0;
}

static int evaluate(Runtime *runtime, Ast *expression) {
    int left;
    int right;
    Symbol *symbol;
    if (runtime->failed) return 0;
    switch (expression->kind) {
        case AST_INT: return expression->as.integer;
        case AST_VAR:
            symbol = find_symbol(runtime, expression->as.name);
            return symbol ? symbol->value : runtime_error(runtime, "undefined variable", expression->as.name);
        case AST_UNARY:
            left = evaluate(runtime, expression->as.operation.left);
            return expression->as.operation.op == OP_NEG ? -left : !left;
        case AST_BINARY:
            left = evaluate(runtime, expression->as.operation.left);
            if (expression->as.operation.op == OP_AND && !left) return 0;
            if (expression->as.operation.op == OP_OR && left) return 1;
            right = evaluate(runtime, expression->as.operation.right);
            switch (expression->as.operation.op) {
                case OP_ADD: return left + right;
                case OP_SUB: return left - right;
                case OP_MUL: return left * right;
                case OP_DIV: return right ? left / right : runtime_error(runtime, "division by zero", NULL);
                case OP_MOD: return right ? left % right : runtime_error(runtime, "modulo by zero", NULL);
                case OP_LT: return left < right;
                case OP_LE: return left <= right;
                case OP_GT: return left > right;
                case OP_GE: return left >= right;
                case OP_EQ: return left == right;
                case OP_NE: return left != right;
                case OP_AND: return left && right;
                case OP_OR: return left || right;
                default: return runtime_error(runtime, "invalid binary operator", NULL);
            }
        default: return runtime_error(runtime, "invalid expression", NULL);
    }
}

static void execute_list(Runtime *runtime, Ast *statement) {
    int value;
    Symbol *symbol;
    while (statement && !runtime->failed && !runtime->returned) {
        switch (statement->kind) {
            case AST_DECL:
                if (find_symbol(runtime, statement->as.binding.name)) {
                    runtime_error(runtime, "variable already declared", statement->as.binding.name);
                    break;
                }
                value = evaluate(runtime, statement->as.binding.value);
                symbol = calloc(1, sizeof(*symbol));
                if (!symbol) { runtime_error(runtime, "out of memory", NULL); break; }
                symbol->name = statement->as.binding.name;
                symbol->value = value;
                symbol->next = runtime->symbols;
                runtime->symbols = symbol;
                break;
            case AST_ASSIGN:
                symbol = find_symbol(runtime, statement->as.binding.name);
                if (!symbol) { runtime_error(runtime, "undefined variable", statement->as.binding.name); break; }
                symbol->value = evaluate(runtime, statement->as.binding.value);
                break;
            case AST_PRINT_EXPR:
                value = evaluate(runtime, statement->as.expression);
                if (!runtime->failed) printf("%d\n", value);
                break;
            case AST_PRINT_STRING:
                puts(statement->as.string);
                break;
            case AST_IF:
                value = evaluate(runtime, statement->as.conditional.condition);
                if (value) execute_list(runtime, statement->as.conditional.then_branch);
                else execute_list(runtime, statement->as.conditional.else_branch);
                break;
            case AST_WHILE:
                while (!runtime->failed && !runtime->returned && evaluate(runtime, statement->as.loop.condition)) {
                    execute_list(runtime, statement->as.loop.body);
                }
                break;
            case AST_RETURN:
                runtime->return_value = evaluate(runtime, statement->as.expression);
                runtime->returned = !runtime->failed;
                break;
            default:
                runtime_error(runtime, "invalid statement", NULL);
                break;
        }
        statement = statement->next;
    }
}

int interpret(Ast *program) {
    Runtime runtime = {0};
    Symbol *symbol;
    execute_list(&runtime, program);
    while (runtime.symbols) {
        symbol = runtime.symbols->next;
        free(runtime.symbols);
        runtime.symbols = symbol;
    }
    return runtime.failed ? 1 : runtime.return_value;
}

