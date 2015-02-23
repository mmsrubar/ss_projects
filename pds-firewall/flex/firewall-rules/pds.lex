/* scanner for a toy Pascal-like language */

%{
#include <stdio.h>
%}

DIGIT    [0-9]
ID       [a-z][a-z0-9]*

%%

{DIGIT}+ printf("CISLO PRAVIDLA|PORT: %s (%d)\n", yytext, atoi(yytext));
allow|deny printf("AKCE: %s\n", yytext);
tcp|udp|icmp|ip        printf( "Protokol: %s\n", yytext );
from|to|src-port|dst-port printf("KEYWORD: %s\n", yytext);
[ \t\n]+          /* eat up whitespace */
%%
//{DIGIT}+  printf("PORT: %s\n", yytext);
