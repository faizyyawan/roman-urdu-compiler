CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS := -Iinclude

.PHONY: all test demo clean

all: bin/roman-urdu

build bin:
	mkdir -p $@

build/parser.tab.c build/parser.tab.h: src/parser.y include/ast.h include/frontend.h | build
	bison -Wall -d -o build/parser.tab.c src/parser.y

build/lexer.yy.c: src/lexer.l build/parser.tab.h | build
	flex -o $@ src/lexer.l

bin/roman-urdu: build/parser.tab.c build/lexer.yy.c src/main.c src/ast.c src/codegen.c include/ast.h include/codegen.h include/frontend.h | bin
	$(CC) $(CFLAGS) $(CPPFLAGS) -Ibuild -o $@ build/parser.tab.c build/lexer.yy.c src/main.c src/ast.c src/codegen.c

test: all
	sh tests/run_tests.sh

demo: all
	bin/roman-urdu interpret examples/demo.ru
	bin/roman-urdu compile examples/demo.ru build/demo.ll
	clang build/demo.ll -o build/demo
	build/demo

clean:
	rm -rf bin build

