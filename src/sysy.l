%option noyywrap
%option nounput
%option noinput
%option yylineno

%{

#include <cstdlib>
#include <iostream>
#include <string>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "ast_defs.hpp"
#include "sysy.tab.hpp"

%}

/* 空白符和注释 */
WhiteSpace		[ \t\n\r]*
LineComment		\/\/.*
MultiLineComment	"/*"([^*]|[*][^/]|[\r\n])*"*/"

/* 标识符 */
Identifier		[a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Binary			0[bB][01]+
Decimal			[1-9][0-9]*
Octal			0[0-7]*
Hexadecimal		0[xX][0-9a-fA-F]+

%%

{WhiteSpace}		{ }
{LineComment}		{ }
{MultiLineComment}	{ }

"const" 			{ return CONST; }
"int"				{ return INT; }
"return"			{ return RETURN; }
"if"				{ return IF; }
"else"				{ return ELSE; }
"while"				{ return WHILE; }
"break"				{ return BREAK; }
"continue"			{ return CONTINUE; }

"+"					{ return ADD; }
"-"					{ return SUB; }
"*"					{ return MUL; }
"/"					{ return DIV; }
"%"					{ return MOD; }

"<="				{ return LE; }
"<"					{ return LT;}
">="				{ return GE; }
">"					{ return GT; }
"=="				{ return EQ; }
"!="				{ return NEQ; }

"&&"				{ return LAND; }
"||"				{ return LOR; }
"!"					{ return LNOT; }


{Identifier}    { yylval.str_val = new std::string(yytext); return IDENT; }

{Binary} 		{ 
	yylval.int_val = strtol(yytext+2, nullptr, 2); 
	return INT_CONST;
}
{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { return yytext[0]; }

%%
