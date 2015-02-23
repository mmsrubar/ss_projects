%{
#include <stdio.h>
%}

%token NUM ACTION PROTOCOL FROM TO SRC_PORT DST_PORT IP

/*
 * <rules>  -> epsilon | <rules> <rule>
 * <rule>   -> NUM ACTION PROTOCOL FROM IP TO IP <ports>
 * <ports>  -> epsilon | src-port NUM | dst-port NUM
rules: /* epsilon  | rule;
 */
%%
rules: /* epsilon */ 
     | 
     rule 
     {
        printf("ctu rules pravidlo\n");};
rule: NUM ACTION PROTOCOL FROM IP TO IP port rules {
    printf("num: %s\n", $1);
    };
port: /* epsilon */ | SRC_PORT NUM | DST_PORT NUM | SRC_PORT NUM DST_PORT NUM;
%%
