CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS := -Iinclude

.PHONY: all phase1 phase2 test demo clean

all: phase1 phase2

phase1: bin/roman-urdu-interpreter

phase2: bin/roman-urdu-compiler

build/phase1 build/phase2 bin:
	mkdir -p $@

build/phase1/parser.tab.c build/phase1/parser.tab.h: phase1/parser.y include/ast.h | build/phase1
	bison -Wall -d -o build/phase1/parser.tab.c phase1/parser.y

build/phase1/lexer.yy.c: phase1/lexer.l build/phase1/parser.tab.h | build/phase1
	flex -o $@ phase1/lexer.l

bin/roman-urdu-interpreter: build/phase1/parser.tab.c build/phase1/lexer.yy.c src/ast.c include/ast.h | bin
	$(CC) $(CFLAGS) $(CPPFLAGS) -Ibuild/phase1 -o $@ build/phase1/parser.tab.c build/phase1/lexer.yy.c src/ast.c

build/phase2/parser.tab.c build/phase2/parser.tab.h: phase2/parser.y include/ast.h include/codegen.h | build/phase2
	bison -Wall -d -o build/phase2/parser.tab.c phase2/parser.y

build/phase2/lexer.yy.c: phase2/lexer.l build/phase2/parser.tab.h | build/phase2
	flex -o $@ phase2/lexer.l

bin/roman-urdu-compiler: build/phase2/parser.tab.c build/phase2/lexer.yy.c src/ast.c src/codegen.c include/ast.h include/codegen.h | bin
	$(CC) $(CFLAGS) $(CPPFLAGS) -Ibuild/phase2 -o $@ build/phase2/parser.tab.c build/phase2/lexer.yy.c src/ast.c src/codegen.c

test: all
	sh tests/run_tests.sh

demo: all
	bin/roman-urdu-interpreter examples/demo.ru
	bin/roman-urdu-compiler examples/demo.ru build/demo.ll
	clang build/demo.ll -o build/demo
	build/demo

clean:
	rm -rf bin build

