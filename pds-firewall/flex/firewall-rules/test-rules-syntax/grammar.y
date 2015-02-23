%{
#include <stdio.h>
#include <string.h>
 
void yyerror(const char *str)
{
        fprintf(stderr,"error: %s\n",str);
}
 
int yywrap()
{
        return 1;
} 
  
main()
{
  printf("ret: %d\n", yyparse());
} 
%}

%token NUM /*ACTION PROTOCOL FROM TO SRC_PORT DST_PORT IP*/

/*
 * <rules>  -> epsilon | <rules> <rule>
 * <rule>   -> NUM ACTION PROTOCOL FROM IP TO IP <ports>
 * <ports>  -> epsilon | src-port NUM | dst-port NUM
rules: /* epsilon  | rule;
 */
%%
rules: /* epsilon */ | NUM;
%%
//rule: NUM ACTION PROTOCOL FROM IP TO IP port rules {
//port: /* epsilon */ | SRC_PORT NUM | DST_PORT NUM | SRC_PORT NUM DST_PORT NUM;
