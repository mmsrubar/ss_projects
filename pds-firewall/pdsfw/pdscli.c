/*
 * Project:     Simple Firewall (PDS)
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Sun Mar  1 00:31:18 CET 2015
 * Description: 
 */
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
  printf("Usage: %s [options] [filter]\n", program_name);

  fputs("\n\
options:\n\
  -f filter-file : Read rules from a file. One rule per a line.\n\
  -p             : Print firewall rules.\n\
  -v             : Verbose mode.\n\
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
}

/* Send print command to the kernel module.
 *
 * @return EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int print_rules()
{
  FILE *f;
  int ch;

  if ((f = fopen(PROCF_NAME, "r")) == NULL) {
    perror("fopen()");

    if (errno == ENOENT) {
      printf("Error: Is the kernel module loaded? (try lsmod|grep pdsfw)\n");
    }

    return EXIT_FAILURE;
  }

  printf("id    action srcip           srcport dstip           dstport protocol\n");
  while ((ch = fgetc(f)) != EOF) {
    printf("%c",ch);
  }

  if (fclose(f) == EOF) {
    perror("fclose()");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Copy content of a file with rule directly into the /proc file. Syntax is
 * checked automatically by yacc before this funcion is called.
 * 
 * @param stat  structure containg program flags
 * @param input file with rules
 *
 * @return EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int add_rules(struct prog_stat *stat, FILE *input)
{
  FILE *output;
  char line[256];

  if ((output = fopen(PROCF_NAME, "w")) == NULL) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  while (fgets(line, sizeof(line), input) != NULL) {
    eprintf(stat->v, "> coping line: %s\n", line);
    fprintf (output, line);
  }

  if (fclose(output) == EOF) {
    perror("fclose()");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Write a single rule which was specified as program argument after -a option
 * into the /proc file.
 *
 * @param stat  structure containg program flags
 * @param rule  rule as a string
 *
 * @return EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int add_single_rule(struct prog_stat *stat, const char *rule)
{
  FILE *f;
  int ret;

  if ((f = fopen(PROCF_NAME, "w")) == NULL) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  if ((ret = fprintf(f, "%s", rule)) < 0) {
    fprintf(stderr, 
        "Error: there has been error while writing rule to the file %s\n", 
        PROCF_NAME);
    return EXIT_FAILURE;
  }

  eprintf(stat->v, "> Buffer (%dB) was written to the file %s\n", ret, PROCF_NAME);

  if (fclose(f) == EOF) {
    fprintf(stderr, 
        "Error: There has been error while closing the file %s\n", PROCF_NAME);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Construct and send a delete command to the kernel side.
 *
 * @param num The id of the rule which will be deleted.
 *
 * @return EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int delete_rule(int num)
{
  FILE *f;

  if ((f = fopen(PROCF_NAME, "w")) == NULL) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  /* write delete command */
  fprintf(f, "delete %d", num);

  if (fclose(f) == EOF) {
    perror("fclose()");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
  char ch;
  FILE *input;
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

        if ((input = fopen(optarg, "r")) == NULL) {
          perror("fopen()");
          return 1;
        }

        yyin = input;
        if (yyparse() == EXIT_SUCCESS) {
          eprintf(stat.v, "> syntax of rules in file %s is OK\n", optarg);
        }

        /* get back to the begining of the file because yacc read it all */
        if (fseek(input, 0, SEEK_SET)) {
          puts("Error seeking to start of file");
          return 1;
        }

        add_rules(&stat, input);

        if (fclose(input) == EOF) {
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
      add_single_rule(&stat, sr_buf);
    }
    yy_delete_buffer(buffer);
  }

  return 0;
//yywrap();
} 
