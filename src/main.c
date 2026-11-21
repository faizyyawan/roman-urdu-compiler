#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ast.h"
#include "codegen.h"
#include "frontend.h"
#include "semantic.h"

static void print_usage(const char *program) {
    fprintf(stderr,
            "usage:\n"
            "  %s interpret program.ru\n"
            "  %s compile program.ru [output.ll]\n"
            "  %s build program.ru executable\n",
            program, program, program);
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
    if (result == 0 && semantic_check(*program) != 0) {
        ast_free(*program);
        *program = NULL;
        return 1;
    }
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

static int build_executable(const char *input_path, const char *output_path) {
    char temporary_path[] = "/tmp/roman-urdu-XXXXXX";
    Ast *program = NULL;
    FILE *temporary;
    pid_t child;
    int descriptor;
    int status;
    int result = load_program(input_path, &program);
    if (result != 0) return result;

    descriptor = mkstemp(temporary_path);
    if (descriptor < 0) {
        perror("could not create temporary LLVM IR file");
        ast_free(program);
        return 2;
    }
    temporary = fdopen(descriptor, "w");
    if (!temporary) {
        perror("could not open temporary LLVM IR file");
        close(descriptor);
        unlink(temporary_path);
        ast_free(program);
        return 2;
    }
    result = emit_llvm_ir(temporary, program);
    if (fclose(temporary) != 0) result = 1;
    ast_free(program);
    if (result != 0) {
        unlink(temporary_path);
        return result;
    }

    child = fork();
    if (child < 0) {
        perror("could not start clang");
        unlink(temporary_path);
        return 2;
    }
    if (child == 0) {
        execlp("clang", "clang", "-Wno-override-module", "-x", "ir", temporary_path,
               "-o", output_path, (char *)NULL);
        perror("could not run clang");
        _exit(127);
    }
    do {
        result = waitpid(child, &status, 0);
    } while (result < 0 && errno == EINTR);
    unlink(temporary_path);
    if (result < 0) {
        perror("could not wait for clang");
        return 2;
    }
    if (!WIFEXITED(status)) {
        fputs("clang terminated unexpectedly\n", stderr);
        return 1;
    }
    return WEXITSTATUS(status);
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "interpret") == 0) {
        return run_interpreter(argv[2]);
    }
    if ((argc == 3 || argc == 4) && strcmp(argv[1], "compile") == 0) {
        return run_compiler(argv[2], argc == 4 ? argv[3] : NULL);
    }
    if (argc == 4 && strcmp(argv[1], "build") == 0) {
        return build_executable(argv[2], argv[3]);
    }
    print_usage(argv[0]);
    return 2;
}

