%{
#include <stdio.h>
%}


%token ID IP ACTION PROTOCOL FROM TO SRCPORTSTR DESTPORTSTR PORT

%%
rules: /* empty */ | rule;
rule: ID ACTION PROTOCOL FROM IP TO IP port rules;
port: /* empty */ | SRCPORTSTR PORT | DESTPORTSTR PORT | SRCPORTSTR PORT DESTPORTSTR PORT;
%%
