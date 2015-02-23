

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "grammar.tab.h"
 
extern FILE *yyin;

void yyerror(const char *str)
{
        fprintf(stderr,"error: %s\n",str);
}
 
int yywrap()
{
  /*
  if ((yyin = fopen("one-rule.in", "r")) != NULL)
    return 0;
  else
  */
    return 1;
} 
  
int main(int argc, char *argv[])
{
  char ch;
  int count = 0;

  while ((ch = getopt(argc, argv, "f:pd:a")) != EOF) {
    switch (ch) {
      case 'f':
        printf("pravidla budu cist ze souboru: %s\n", optarg);
        if ((yyin = fopen("one-rule.in", "r")) != NULL)
          printf("ret: %d\n", yyparse());
        else 
          printf("cant open file\n");
        fclose(yyin);
        break;
      case 'p':
        puts("print pravidel");
        return 0;
      case 'd':
        printf("odstraneni pravidla: %d\n", atoi(optarg));
        return 0;
      case 'a':
        printf("budu pridavat nove pravidlo\n");
        printf("ret: %d\n", yyparse());
        break;
      default:
        fprintf(stderr, "Unknown option: '%s'\n", optarg);
        return 1;
    }
  }

  argc -= optind;
  argv += optind;
  for (count = 0; count < argc; count++)
    printf(" - %s\n", argv[count]);

yywrap();
  printf("arg: %s\n", argv[1]);
} 
