#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "grammar.tab.h"
 
#define PROCF_NAME  "/proc/pdsfw_xsruba03"
#define BUF_SIZE    124

#define eprintf(ok_to_print, format, ...) do {                 \
    if (ok_to_print)                              \
        fprintf(stderr, format, ##__VA_ARGS__);   \
} while(0)

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int yyparse();
extern YY_BUFFER_STATE yy_scan_string(char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

extern FILE *yyin;
extern int yylex(void);

struct prog_stat {
  bool v;         /* verbose mode */
  bool add_rule;
};

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
  
void usage(const char *program_name)
{
  //printf("Usage: %s [options] [filter]
  printf("Usage: %s [options] [filter]\n", program_name);

  fputs("\n\
options:\n\
  -f filter-file : Read rules from a file. One rule per a line.\n\
  -p             : Print firewall rules.\n\
\n\
filter:\n\
  -a rule    : Add rule.\n\
  -d rule-id : Remove rule by its ID.\n\
", stdout);


  fputs("\n\
Rules has to have following format:\n\
<number> <action> <protocol> from <src IP> to <dest IP> [src-port <srcPort>] [dst-port <dstPort>]\n\
\n\
<number>   : number of a rule\n\
<action>   : allow | deny\n\
<protocol> : tcp | udp | icmp | ip\n\
<src IP>   : IPv4 address | any\n\
<dst IP>   : IPv4 adresa | any\n\
<srcPort>  : port number\n\
<dstPort>  : port number\n\
", stdout);

  /*
      fputs("\
\n\
By default, sparse SOURCE files are detected by a crude heuristic and the\n\
corresponding DEST file is made sparse as well.  That is the behavior\n\
selected by --sparse=auto.  Specify --sparse=always to create a sparse DEST\n\
file whenever the SOURCE file contains a long enough sequence of zero bytes.\n\
Use --sparse=never to inhibit creation of sparse files.\n\
\n\
When --reflink[=always] is specified, perform a lightweight copy, where the\n\
data blocks are copied only when modified.  If this is not possible the copy\n\
fails, or if --reflink=auto is specified, fall back to a standard copy.\n\
", stdout);
*/
}

int print_rules()
{
  FILE *f;
  int ch;

  if ((f = fopen(PROCF_NAME, "r")) == NULL) {
    perror("fopen()");

    if (errno == ENOENT) {
      printf("ERROR: Is the kernel module loaded? (try lsmod|grep pdsfw)\n");
    }

    return 1;
  }

  printf("id action srcip srcport dstip dstport protocol\n");
  while ((ch = fgetc(f)) != EOF) {
    printf("%c",ch);
  }

  if (fclose(f) == EOF) {
    perror("fclose()");
    return 1;
  }

  return 0;
}

int add_rule(struct prog_stat *stat, const char *rule)
{
  FILE *f;
  int ret;

  if ((f = fopen(PROCF_NAME, "w")) == NULL) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  if ((ret = fprintf(f, "%s", rule)) < 0) {
    fprintf(stderr, 
        "ERROR: there has been error while writing rule to the file %s\n", 
        PROCF_NAME);
    return EXIT_FAILURE;
  }

  eprintf(stat->v, "> Buffer (%dB) was written to the file %s\n", ret, PROCF_NAME);

  if (fclose(f) == EOF) {
    fprintf(stderr, 
        "ERROR: There has been error while closing the file %s\n", PROCF_NAME);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int delete_rule(int num)
{
  FILE *f;

  if ((f = fopen(PROCF_NAME, "w")) == NULL) {
    perror("fopen()");
    return 1;
  }

  /* write delete command */
  fprintf(f, "delete %d", num);

  if (fclose(f) == EOF) {
    perror("fclose()");
    return 1;
  }

  return 0;
}

int main(int argc, char *argv[])
{
  char ch;
  int count = 0;
  /* buffer for single rule from command line (-a option) */
  char sr_buf[BUF_SIZE] = {[BUF_SIZE-1] = '\0'};
  struct prog_stat stat = {.v= false, .add_rule = false};

  if (argc <= 1) {
    usage(argv[0]);
    return 0;
  }

  while ((ch = getopt(argc, argv, "hf:pd:av")) != EOF) {
    switch (ch) {
      case 'h':
        usage(argv[0]);
        return 0;
      case 'v':
        stat.v = true;
        eprintf(stat.v, "> verbose mode: on\n");
        break;
      case 'f':
        eprintf(stat.v, "> firewall policy rules will be taken from the file %s", optarg);

        if ((yyin = fopen(optarg, "r")) == NULL) {
          perror("fopen()");
          return 1;
        }

        if (yyparse() == EXIT_SUCCESS) {
          eprintf(stat.v, "> syntax of rules in file %s is OK\n", optarg);
        }

        if (fclose(yyin) == EOF) {
          perror("fclose()");
          return 1;
        }
        break;
      case 'p':
        eprintf(stat.v, "> Reading rules from %s\n", PROCF_NAME);
        print_rules();
        return 0;
      case 'd':
        eprintf(stat.v, "> Removing rule with id=%d\n", atoi(optarg));
        delete_rule(atoi(optarg));
        return 0;
      case 'a':
        eprintf(stat.v, "> Addding a new rule\n");
        stat.add_rule = true;
        break;
      default:
        fprintf(stderr, "Unknown option: '%s'\n", optarg);
        return 1;
    }
  }

  argc -= optind;
  argv += optind;

  if (stat.add_rule) {
    /* 1. check the syntax of a rule
     * 2. write it into the buffer
     * 3. write it directly into the file?
     */
    for (count = 0; count < argc; count++) {
      strcat(sr_buf, argv[count]);
      strcat(sr_buf, " ");
    }

    YY_BUFFER_STATE buffer = yy_scan_string(sr_buf);
    if (yyparse() == EXIT_SUCCESS) {
      eprintf(stat.v, "> syntax of the rule \'%s\' is OK\n", sr_buf);
      add_rule(&stat, sr_buf);
    }
    yy_delete_buffer(buffer);
  }

  return 0;
//yywrap();
} 
