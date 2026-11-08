#ifndef ROMAN_URDU_CODEGEN_H
#define ROMAN_URDU_CODEGEN_H

#include <stdio.h>
#include "ast.h"

int emit_llvm_ir(FILE *output, Ast *program);

#endif

