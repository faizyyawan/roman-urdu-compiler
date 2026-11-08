#ifndef ROMAN_URDU_AST_H
#define ROMAN_URDU_AST_H

typedef enum {
    AST_INT,
    AST_VAR,
    AST_BINARY,
    AST_UNARY,
    AST_DECL,
    AST_ASSIGN,
    AST_PRINT_EXPR,
    AST_PRINT_STRING,
    AST_IF,
    AST_WHILE,
    AST_RETURN
} AstKind;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_EQ,
    OP_NE,
    OP_AND,
    OP_OR,
    OP_NEG,
    OP_NOT
} AstOp;

typedef struct Ast Ast;

struct Ast {
    AstKind kind;
    Ast *next;
    union {
        int integer;
        char *name;
        char *string;
        struct { AstOp op; Ast *left; Ast *right; } operation;
        struct { char *name; Ast *value; } binding;
        struct { Ast *condition; Ast *then_branch; Ast *else_branch; } conditional;
        struct { Ast *condition; Ast *body; } loop;
        Ast *expression;
    } as;
};

Ast *ast_integer(int value);
Ast *ast_variable(char *name);
Ast *ast_binary(AstOp op, Ast *left, Ast *right);
Ast *ast_unary(AstOp op, Ast *operand);
Ast *ast_binding(AstKind kind, char *name, Ast *value);
Ast *ast_expression_statement(AstKind kind, Ast *expression);
Ast *ast_print_string(char *text);
Ast *ast_if(Ast *condition, Ast *then_branch, Ast *else_branch);
Ast *ast_while(Ast *condition, Ast *body);
Ast *ast_append(Ast *list, Ast *node);
void ast_free(Ast *node);

int interpret(Ast *program);

#endif

