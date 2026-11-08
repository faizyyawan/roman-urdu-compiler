#include "codegen.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Variable Variable;
typedef struct StringConstant StringConstant;

struct Variable {
    const char *name;
    Variable *next;
};

struct StringConstant {
    Ast *node;
    int id;
    size_t length;
    StringConstant *next;
};

typedef struct {
    FILE *output;
    Variable *variables;
    StringConstant *strings;
    int next_temporary;
    int next_label;
    int failed;
} Generator;

typedef struct {
    char text[32];
} Value;

static void codegen_error(Generator *generator, const char *format, ...) {
    va_list arguments;
    if (generator->failed) return;
    fputs("code generation error: ", stderr);
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
    generator->failed = 1;
}

static Variable *find_variable(Generator *generator, const char *name) {
    Variable *variable;
    for (variable = generator->variables; variable; variable = variable->next) {
        if (strcmp(variable->name, name) == 0) return variable;
    }
    return NULL;
}

static void add_variable(Generator *generator, const char *name) {
    Variable *variable;
    Variable **tail;
    if (find_variable(generator, name)) {
        codegen_error(generator, "variable already declared: %s", name);
        return;
    }
    variable = calloc(1, sizeof(*variable));
    if (!variable) { codegen_error(generator, "out of memory"); return; }
    variable->name = name;
    tail = &generator->variables;
    while (*tail) tail = &(*tail)->next;
    *tail = variable;
}

static void add_string(Generator *generator, Ast *node) {
    StringConstant *string = calloc(1, sizeof(*string));
    StringConstant **tail;
    if (!string) { codegen_error(generator, "out of memory"); return; }
    string->node = node;
    string->length = strlen(node->as.string) + 1;
    tail = &generator->strings;
    while (*tail) tail = &(*tail)->next;
    string->id = *tail ? (*tail)->id + 1 : 0;
    if (generator->strings) {
        StringConstant *last = generator->strings;
        while (last->next) last = last->next;
        string->id = last->id + 1;
    }
    *tail = string;
}

static void collect_program(Generator *generator, Ast *statement) {
    while (statement && !generator->failed) {
        switch (statement->kind) {
            case AST_DECL:
                add_variable(generator, statement->as.binding.name);
                break;
            case AST_PRINT_STRING:
                add_string(generator, statement);
                break;
            case AST_IF:
                collect_program(generator, statement->as.conditional.then_branch);
                collect_program(generator, statement->as.conditional.else_branch);
                break;
            case AST_WHILE:
                collect_program(generator, statement->as.loop.body);
                break;
            default:
                break;
        }
        statement = statement->next;
    }
}

static int temporary(Generator *generator) {
    return generator->next_temporary++;
}

static int label(Generator *generator) {
    return generator->next_label++;
}

static Value immediate(int integer) {
    Value value;
    snprintf(value.text, sizeof(value.text), "%d", integer);
    return value;
}

static Value register_value(int number) {
    Value value;
    snprintf(value.text, sizeof(value.text), "%%t%d", number);
    return value;
}

static Value emit_expression(Generator *generator, Ast *expression) {
    Value left = immediate(0);
    Value right;
    int first;
    int second;
    const char *instruction = NULL;
    if (generator->failed) return left;
    switch (expression->kind) {
        case AST_INT:
            return immediate(expression->as.integer);
        case AST_VAR:
            if (!find_variable(generator, expression->as.name)) {
                codegen_error(generator, "undefined variable: %s", expression->as.name);
                return left;
            }
            first = temporary(generator);
            fprintf(generator->output, "  %%t%d = load i32, ptr %%v_%s\n", first, expression->as.name);
            return register_value(first);
        case AST_UNARY:
            left = emit_expression(generator, expression->as.operation.left);
            first = temporary(generator);
            if (expression->as.operation.op == OP_NEG) {
                fprintf(generator->output, "  %%t%d = sub i32 0, %s\n", first, left.text);
                return register_value(first);
            }
            fprintf(generator->output, "  %%t%d = icmp eq i32 %s, 0\n", first, left.text);
            second = temporary(generator);
            fprintf(generator->output, "  %%t%d = zext i1 %%t%d to i32\n", second, first);
            return register_value(second);
        case AST_BINARY:
            left = emit_expression(generator, expression->as.operation.left);
            right = emit_expression(generator, expression->as.operation.right);
            switch (expression->as.operation.op) {
                case OP_ADD: instruction = "add"; break;
                case OP_SUB: instruction = "sub"; break;
                case OP_MUL: instruction = "mul"; break;
                case OP_DIV: instruction = "sdiv"; break;
                case OP_MOD: instruction = "srem"; break;
                default: break;
            }
            if (instruction) {
                first = temporary(generator);
                fprintf(generator->output, "  %%t%d = %s i32 %s, %s\n", first, instruction, left.text, right.text);
                return register_value(first);
            }
            if (expression->as.operation.op == OP_AND || expression->as.operation.op == OP_OR) {
                first = temporary(generator);
                fprintf(generator->output, "  %%t%d = icmp ne i32 %s, 0\n", first, left.text);
                second = temporary(generator);
                fprintf(generator->output, "  %%t%d = icmp ne i32 %s, 0\n", second, right.text);
                {
                    int combined = temporary(generator);
                    int widened = temporary(generator);
                    fprintf(generator->output, "  %%t%d = %s i1 %%t%d, %%t%d\n", combined,
                            expression->as.operation.op == OP_AND ? "and" : "or", first, second);
                    fprintf(generator->output, "  %%t%d = zext i1 %%t%d to i32\n", widened, combined);
                    return register_value(widened);
                }
            }
            switch (expression->as.operation.op) {
                case OP_LT: instruction = "slt"; break;
                case OP_LE: instruction = "sle"; break;
                case OP_GT: instruction = "sgt"; break;
                case OP_GE: instruction = "sge"; break;
                case OP_EQ: instruction = "eq"; break;
                case OP_NE: instruction = "ne"; break;
                default: codegen_error(generator, "invalid binary operator"); return left;
            }
            first = temporary(generator);
            fprintf(generator->output, "  %%t%d = icmp %s i32 %s, %s\n", first, instruction, left.text, right.text);
            second = temporary(generator);
            fprintf(generator->output, "  %%t%d = zext i1 %%t%d to i32\n", second, first);
            return register_value(second);
        default:
            codegen_error(generator, "invalid expression node");
            return left;
    }
}

static StringConstant *find_string(Generator *generator, Ast *node) {
    StringConstant *string;
    for (string = generator->strings; string; string = string->next) {
        if (string->node == node) return string;
    }
    return NULL;
}

static int emit_statements(Generator *generator, Ast *statement) {
    Value value;
    int condition;
    int then_label;
    int else_label;
    int end_label;
    int then_terminated;
    int else_terminated;
    StringConstant *string;
    while (statement && !generator->failed) {
        switch (statement->kind) {
            case AST_DECL:
            case AST_ASSIGN:
                if (!find_variable(generator, statement->as.binding.name)) {
                    codegen_error(generator, "undefined variable: %s", statement->as.binding.name);
                    return 0;
                }
                value = emit_expression(generator, statement->as.binding.value);
                fprintf(generator->output, "  store i32 %s, ptr %%v_%s\n", value.text, statement->as.binding.name);
                break;
            case AST_PRINT_EXPR:
                value = emit_expression(generator, statement->as.expression);
                fprintf(generator->output,
                        "  call i32 (ptr, ...) @printf(ptr @.fmt.int, i32 %s)\n", value.text);
                break;
            case AST_PRINT_STRING:
                string = find_string(generator, statement);
                if (!string) { codegen_error(generator, "missing string constant"); return 0; }
                fprintf(generator->output,
                        "  call i32 (ptr, ...) @printf(ptr @.fmt.str, ptr @.str.%d)\n", string->id);
                break;
            case AST_IF:
                value = emit_expression(generator, statement->as.conditional.condition);
                condition = temporary(generator);
                then_label = label(generator);
                else_label = label(generator);
                end_label = label(generator);
                fprintf(generator->output, "  %%t%d = icmp ne i32 %s, 0\n", condition, value.text);
                fprintf(generator->output, "  br i1 %%t%d, label %%if.then.%d, label %%if.else.%d\n\n",
                        condition, then_label, else_label);
                fprintf(generator->output, "if.then.%d:\n", then_label);
                then_terminated = emit_statements(generator, statement->as.conditional.then_branch);
                if (!then_terminated) fprintf(generator->output, "  br label %%if.end.%d\n", end_label);
                fprintf(generator->output, "\nif.else.%d:\n", else_label);
                else_terminated = emit_statements(generator, statement->as.conditional.else_branch);
                if (!else_terminated) fprintf(generator->output, "  br label %%if.end.%d\n", end_label);
                if (then_terminated && else_terminated && statement->as.conditional.else_branch) return 1;
                fprintf(generator->output, "\nif.end.%d:\n", end_label);
                break;
            case AST_WHILE:
                then_label = label(generator);
                else_label = label(generator);
                end_label = label(generator);
                fprintf(generator->output, "  br label %%while.cond.%d\n\n", then_label);
                fprintf(generator->output, "while.cond.%d:\n", then_label);
                value = emit_expression(generator, statement->as.loop.condition);
                condition = temporary(generator);
                fprintf(generator->output, "  %%t%d = icmp ne i32 %s, 0\n", condition, value.text);
                fprintf(generator->output, "  br i1 %%t%d, label %%while.body.%d, label %%while.end.%d\n\n",
                        condition, else_label, end_label);
                fprintf(generator->output, "while.body.%d:\n", else_label);
                if (!emit_statements(generator, statement->as.loop.body)) {
                    fprintf(generator->output, "  br label %%while.cond.%d\n", then_label);
                }
                fprintf(generator->output, "\nwhile.end.%d:\n", end_label);
                break;
            case AST_RETURN:
                value = emit_expression(generator, statement->as.expression);
                fprintf(generator->output, "  ret i32 %s\n", value.text);
                return 1;
            default:
                codegen_error(generator, "invalid statement node");
                return 0;
        }
        statement = statement->next;
    }
    return 0;
}

static void emit_escaped_string(FILE *output, const char *text) {
    const unsigned char *character = (const unsigned char *)text;
    while (*character) {
        if (*character >= 32 && *character <= 126 && *character != '"' && *character != '\\') {
            fputc(*character, output);
        } else {
            fprintf(output, "\\%02X", *character);
        }
        ++character;
    }
    fputs("\\00", output);
}

static void free_generator(Generator *generator) {
    while (generator->variables) {
        Variable *next = generator->variables->next;
        free(generator->variables);
        generator->variables = next;
    }
    while (generator->strings) {
        StringConstant *next = generator->strings->next;
        free(generator->strings);
        generator->strings = next;
    }
}

int emit_llvm_ir(FILE *output, Ast *program) {
    Generator generator = {0};
    Variable *variable;
    StringConstant *string;
    int terminated;
    generator.output = output;
    collect_program(&generator, program);
    if (generator.failed) { free_generator(&generator); return 1; }

    fputs("; Generated by Roman Urdu Compiler\n", output);
    fputs("declare i32 @printf(ptr, ...)\n\n", output);
    fputs("@.fmt.int = private unnamed_addr constant [4 x i8] c\"%d\\0A\\00\"\n", output);
    fputs("@.fmt.str = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\"\n", output);
    for (string = generator.strings; string; string = string->next) {
        fprintf(output, "@.str.%d = private unnamed_addr constant [%zu x i8] c\"", string->id, string->length);
        emit_escaped_string(output, string->node->as.string);
        fputs("\"\n", output);
    }
    fputs("\ndefine i32 @main() {\nentry:\n", output);
    for (variable = generator.variables; variable; variable = variable->next) {
        fprintf(output, "  %%v_%s = alloca i32\n", variable->name);
    }
    terminated = emit_statements(&generator, program);
    if (!terminated && !generator.failed) fputs("  ret i32 0\n", output);
    fputs("}\n", output);
    if (ferror(output)) codegen_error(&generator, "could not write LLVM IR");
    free_generator(&generator);
    return generator.failed ? 1 : 0;
}

