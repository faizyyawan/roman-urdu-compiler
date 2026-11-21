#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Name Name;
struct Name {
    const char *text;
    Name *next;
};

typedef struct {
    Name *names;
    int failed;
} Checker;

static Name *find_name(Checker *checker, const char *text) {
    Name *name;
    for (name = checker->names; name; name = name->next) {
        if (strcmp(name->text, text) == 0) return name;
    }
    return NULL;
}

static void semantic_error(Checker *checker, const char *message, const char *name) {
    if (checker->failed) return;
    fprintf(stderr, "semantic error: %s%s%s\n", message,
            name ? " " : "", name ? name : "");
    checker->failed = 1;
}

static void add_name(Checker *checker, const char *text) {
    Name *name;
    if (find_name(checker, text)) {
        semantic_error(checker, "variable already declared", text);
        return;
    }
    name = malloc(sizeof(*name));
    if (!name) {
        semantic_error(checker, "out of memory", NULL);
        return;
    }
    name->text = text;
    name->next = checker->names;
    checker->names = name;
}

static void check_expression(Checker *checker, Ast *expression) {
    if (!expression || checker->failed) return;
    switch (expression->kind) {
        case AST_INT:
            break;
        case AST_VAR:
            if (!find_name(checker, expression->as.name)) {
                semantic_error(checker, "variable used before declaration", expression->as.name);
            }
            break;
        case AST_UNARY:
            check_expression(checker, expression->as.operation.left);
            break;
        case AST_BINARY:
            check_expression(checker, expression->as.operation.left);
            check_expression(checker, expression->as.operation.right);
            break;
        default:
            semantic_error(checker, "invalid expression", NULL);
            break;
    }
}

static void check_statements(Checker *checker, Ast *statement, int top_level) {
    while (statement && !checker->failed) {
        switch (statement->kind) {
            case AST_DECL:
                if (!top_level) {
                    semantic_error(checker, "variable declarations are only allowed at top level",
                                   statement->as.binding.name);
                    break;
                }
                check_expression(checker, statement->as.binding.value);
                if (!checker->failed) add_name(checker, statement->as.binding.name);
                break;
            case AST_ASSIGN:
                if (!find_name(checker, statement->as.binding.name)) {
                    semantic_error(checker, "variable assigned before declaration",
                                   statement->as.binding.name);
                    break;
                }
                check_expression(checker, statement->as.binding.value);
                break;
            case AST_PRINT_EXPR:
            case AST_RETURN:
                check_expression(checker, statement->as.expression);
                break;
            case AST_PRINT_STRING:
                break;
            case AST_IF:
                check_expression(checker, statement->as.conditional.condition);
                check_statements(checker, statement->as.conditional.then_branch, 0);
                check_statements(checker, statement->as.conditional.else_branch, 0);
                break;
            case AST_WHILE:
                check_expression(checker, statement->as.loop.condition);
                check_statements(checker, statement->as.loop.body, 0);
                break;
            default:
                semantic_error(checker, "invalid statement", NULL);
                break;
        }
        statement = statement->next;
    }
}

int semantic_check(Ast *program) {
    Checker checker = {0};
    Name *next;
    check_statements(&checker, program, 1);
    while (checker.names) {
        next = checker.names->next;
        free(checker.names);
        checker.names = next;
    }
    return checker.failed ? 1 : 0;
}

