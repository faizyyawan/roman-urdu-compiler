%option noyywrap yylineno nodefault nounput noinput never-interactive

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

static char *copy_text(const char *text, size_t length) {
    char *copy = malloc(length + 1);
    if (!copy) { fputs("fatal: out of memory\n", stderr); exit(2); }
    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

static char *decode_string(const char *text, size_t length) {
    char *result = malloc(length);
    size_t source;
    size_t target = 0;
    if (!result) { fputs("fatal: out of memory\n", stderr); exit(2); }
    for (source = 1; source + 1 < length; ++source) {
        if (text[source] == '\\' && source + 1 < length - 1) {
            ++source;
            if (text[source] == 'n') result[target++] = '\n';
            else if (text[source] == 't') result[target++] = '\t';
            else result[target++] = text[source];
        } else result[target++] = text[source];
    }
    result[target] = '\0';
    return result;
}
%}

%%

[ \t\r]+                 ;
\n                       ;
"//"[^\n]*               ;
"#"[^\n]*                ;
"adad"                   return ADAD;
"agar"                   return AGAR;
"warna"                  return WARNA;
"jabtak"                 return JABTAK;
"dikhao"                 return DIKHAO;
"wapis"                  return WAPIS;
"kardo"                  return KARDO;
"<="                     return LE;
">="                     return GE;
"=="                     return EQ;
"!="                     return NE;
"&&"                     return AND;
"||"                     return OR;
[0-9]+                   { yylval.integer = atoi(yytext); return NUMBER; }
\"([^\"\\]|\\.)*\"       { yylval.text = decode_string(yytext, (size_t)yyleng); return STRING; }
[A-Za-z_][A-Za-z0-9_]*  { yylval.text = copy_text(yytext, (size_t)yyleng); return IDENTIFIER; }
[(){}=+*/%<>!-]          return yytext[0];
.                        { fprintf(stderr, "line %d: unexpected character '%s'\n", yylineno, yytext); return yytext[0]; }

%%

