%code requires {
#include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern FILE *yyin;
extern int yylineno;
int yylex(void);
void yyerror(const char *message);
static Ast *program_root;
%}

%define parse.error detailed

%union {
    int integer;
    char *text;
    Ast *node;
}

%token <integer> NUMBER
%token <text> IDENTIFIER STRING
%token ADAD AGAR WARNA JABTAK DIKHAO WAPIS KARDO
%token LE GE EQ NE AND OR
%type <node> program statement_list statement block optional_else expression

%left OR
%left AND
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%precedence '!' UMINUS

%%

program:
    statement_list { program_root = $1; $$ = $1; }
;

statement_list:
    %empty { $$ = NULL; }
  | statement_list statement { $$ = ast_append($1, $2); }
;

statement:
    ADAD IDENTIFIER '=' expression KARDO { $$ = ast_binding(AST_DECL, $2, $4); }
  | IDENTIFIER '=' expression KARDO { $$ = ast_binding(AST_ASSIGN, $1, $3); }
  | DIKHAO '(' expression ')' KARDO { $$ = ast_expression_statement(AST_PRINT_EXPR, $3); }
  | DIKHAO '(' STRING ')' KARDO { $$ = ast_print_string($3); }
  | AGAR '(' expression ')' block optional_else { $$ = ast_if($3, $5, $6); }
  | JABTAK '(' expression ')' block { $$ = ast_while($3, $5); }
  | WAPIS expression KARDO { $$ = ast_expression_statement(AST_RETURN, $2); }
;

block:
    '{' statement_list '}' { $$ = $2; }
;

optional_else:
    %empty { $$ = NULL; }
  | WARNA block { $$ = $2; }
;

expression:
    NUMBER { $$ = ast_integer($1); }
  | IDENTIFIER { $$ = ast_variable($1); }
  | '(' expression ')' { $$ = $2; }
  | expression '+' expression { $$ = ast_binary(OP_ADD, $1, $3); }
  | expression '-' expression { $$ = ast_binary(OP_SUB, $1, $3); }
  | expression '*' expression { $$ = ast_binary(OP_MUL, $1, $3); }
  | expression '/' expression { $$ = ast_binary(OP_DIV, $1, $3); }
  | expression '%' expression { $$ = ast_binary(OP_MOD, $1, $3); }
  | expression '<' expression { $$ = ast_binary(OP_LT, $1, $3); }
  | expression LE expression { $$ = ast_binary(OP_LE, $1, $3); }
  | expression '>' expression { $$ = ast_binary(OP_GT, $1, $3); }
  | expression GE expression { $$ = ast_binary(OP_GE, $1, $3); }
  | expression EQ expression { $$ = ast_binary(OP_EQ, $1, $3); }
  | expression NE expression { $$ = ast_binary(OP_NE, $1, $3); }
  | expression AND expression { $$ = ast_binary(OP_AND, $1, $3); }
  | expression OR expression { $$ = ast_binary(OP_OR, $1, $3); }
  | '-' expression %prec UMINUS { $$ = ast_unary(OP_NEG, $2); }
  | '!' expression { $$ = ast_unary(OP_NOT, $2); }
;

%%

void yyerror(const char *message) {
    fprintf(stderr, "line %d: %s\n", yylineno, message);
}

int main(int argc, char **argv) {
    int result;
    if (argc != 2) {
        fprintf(stderr, "usage: %s program.ru\n", argv[0]);
        return 2;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror(argv[1]);
        return 2;
    }
    result = yyparse();
    fclose(yyin);
    if (result != 0) {
        ast_free(program_root);
        return 1;
    }
    result = interpret(program_root);
    ast_free(program_root);
    return result;
}

