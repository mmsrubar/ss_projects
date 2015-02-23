%{
#include <stdio.h>
#include <string.h>
#include "grammar.tab.h"
%}

%%
[0-9]+          return NUM;
allow|deny      return ACTION;
tcp|udp|icmp|ip return PROTOCOL;
[0-9]+.[0-9]+.[0-9]+.[0-9]+|any return IP;
from            return FROM;
to              return TO;
src-port        return SRC_PORT;
dst-port        return DST_PORT;
\n                    putchar('\n');  /* ignore EOL */;
[ \t]+                  /* ignore whitespace */;
%%
