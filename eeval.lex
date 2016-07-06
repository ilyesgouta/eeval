
/* Expression Eval Library, (C) Ilyes Gouta, 2007. */

%{

#include <stdio.h>
#include <string.h>
#include <math.h>

#define YYSTYPE_IS_DECLARED
#include <eevaltype.h>

#include <eevalint.h>
#define YY_EXTRA_TYPE eevalctx*

#include <frontend.h>

static int get_token(yyscan_t scanner, char* text);
static int get_id(YYSTYPE *lval, char* text);
static int get_const_int(YYSTYPE *lval, char* text);
static int get_const_float(YYSTYPE *lval, char* text);

%}

%option bison-bridge
%option reentrant
%option noyywrap
%option nounput

DIGIT    [0-9]
ID       [a-z][a-z0-9]*

%%

[ \t\r\n]+

-?{DIGIT}+"."{DIGIT}*    { return (get_const_float(yylval, yytext)); }
-?{DIGIT}+               { return (get_const_int(yylval, yytext)); }

"||"                     { return LOR; }
"&&"                     { return LAND; }

"=="                     { return EQ; }
"!="                     { return NE; }

"<="                     { return LE; }
">="                     { return GE; }

"<"                      { return '<'; }
">"                      { return '>'; }

"+"                      { return '+'; }
"-"                      { return '-'; }
"*"                      { return '*'; }
"/"                      { return '/'; }

"|"                      { return '|'; }
"&"                      { return '&'; }
"!"                      { return '!'; }
"^"                      { return '^'; }
"~"                      { return '~'; }

"="                      { return '='; }

"{"                      { return '{'; }
"}"                      { return '}'; }

"("                      { return '('; }
")"                      { return ')'; }

"["                      { return '['; }
"]"                      { return ']'; }

";"                      { return ';'; }
","                      { return ','; }

"if"                     { return IF; }
"else"                   { return ELSE; }
"for"                    { return FOR; }
"while"                  { return WHILE; }
"do"                     { return DO; }

"int"                    { return INTDECL; }
"float"                  { return FLOATDECL; }

"main"                   { return MAIN; }
"print"                  { return PRINT; }

\"[^\"]*\"               { return (get_token(yyscanner, yytext)); }

{ID}                     { return (get_id(yylval, yytext)); }

%%

/* TODO: if the length of the token is more than 31, return an error. */
static int get_token(yyscan_t scanner, char* text)
{
    eevalctx* pctx = yyget_extra(scanner);
    
    memset(pctx->miscbuf, 0, sizeof(pctx->miscbuf));
    strncpy(pctx->miscbuf, text + 1, sizeof(pctx->miscbuf) - 2);
    
    return TEXT;
}

static int get_id(YYSTYPE *lval, char* text)
{
    memset(lval->id, 0, sizeof(identifier));
    memset(lval->id->name, 0, sizeof(lval->id->name));
    strncpy(lval->id->name, text, sizeof(lval->id->name) - 1);
    
    return IDENTIFIER;
}

static int get_const_int(YYSTYPE *lval, char* text)
{
    memset(lval->id, 0, sizeof(identifier));
    
    lval->id->value.ivalue = atoi(text);
    lval->id->type = CONST_INT_TYPE;
    
    return CONSTINT;
}

static int get_const_float(YYSTYPE *lval, char* text)
{
    memset(lval->id, 0, sizeof(identifier));
    
    lval->id->value.fvalue = atof(text);
    lval->id->type = CONST_FLOAT_TYPE;
    
    return CONSTFLOAT;
}
