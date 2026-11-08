#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "codegen.h"
#include "frontend.h"

static void print_usage(const char *program) {
    fprintf(stderr,
            "usage:\n"
            "  %s interpret program.ru\n"
            "  %s compile program.ru [output.ll]\n",
            program, program);
}

static int load_program(const char *path, Ast **program) {
    FILE *input = fopen(path, "r");
    int result;
    if (!input) {
        perror(path);
        return 2;
    }
    result = parse_program(input, program);
    fclose(input);
    return result;
}

static int run_interpreter(const char *path) {
    Ast *program = NULL;
    int result = load_program(path, &program);
    if (result != 0) return result;
    result = interpret(program);
    ast_free(program);
    return result;
}

static int run_compiler(const char *input_path, const char *output_path) {
    Ast *program = NULL;
    FILE *output = stdout;
    int result = load_program(input_path, &program);
    if (result != 0) return result;
    if (output_path) {
        output = fopen(output_path, "w");
        if (!output) {
            perror(output_path);
            ast_free(program);
            return 2;
        }
    }
    result = emit_llvm_ir(output, program);
    if (output != stdout && fclose(output) != 0) result = 1;
    ast_free(program);
    return result;
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "interpret") == 0) {
        return run_interpreter(argv[2]);
    }
    if ((argc == 3 || argc == 4) && strcmp(argv[1], "compile") == 0) {
        return run_compiler(argv[2], argc == 4 ? argv[3] : NULL);
    }
    print_usage(argv[0]);
    return 2;
}

