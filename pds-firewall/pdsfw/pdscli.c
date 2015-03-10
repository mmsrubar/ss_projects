/*
 * Project:     Simple Firewall (PDS)
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Sun Mar  1 00:31:18 CET 2015
 *
 * Description: 
 * This is a client side for a simple firewall. It was created as a school project
 * for PDS seminar. The following article was used a tutorial for this project:
 * http://www.roman10.net/a-linux-firewall-using-netfilter-part-1overview/
 *
 * FIXME:
 * - if the file with rules is too big then flex wount handle it so it would be
 *   better to read rules line by line and also give to flex line by line?
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
#define ID_MAX      1000000
#define ID_MIN      1

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

static const char *program_name;

struct cmdline_options {
  /* flag for each command line option */
  bool v; /* verbose mode */
  bool a; /* add rule */ 
  bool p; /* print rules */
  bool d; /* delete a rule */
  bool f; /* read rules from a file */

  int id;
  const char *file;
};

void yyerror(const char *str)
{
  fprintf(stderr,"Rule error: %s\n", str);
  fprintf(stderr,"Try %s -h to see the format of a rule\n", program_name);
}
 
int yywrap()
{
  return 1;
} 
  
void usage()
{
  fputs("\
This is a user space command line application for a simple firewall. It\n\
communicates with a kernel module through the /proc file system. Kernel \n\
module provides packet classification based on rules which can be specified\n\
with this utility.\n", stdout);

  printf("Usage: %s [options] [filter]\n", program_name);

  fputs("\n\
options:\n\
  -f filter-file : Read rules from a file. One rule per a line.\n\
  -p             : Print firewall rules.\n\
  -v             : Verbose mode.\n\
\n\
filter:\n\
  -a rule    : Add rule.\n\
  -d rule-id : Remove rule by its ID. Range of IDs is (1, 1000000)\n\
", stdout);


  fputs("\n\
Rules has to have the following format:\n\
<number> <action> <protocol> from <src IP> to <dest IP> [src-port <srcPort>] [dst-port <dstPort>]\n\
\n\
<number>   : number of a rule in range of (1, 1000000)\n\
<action>   : allow | deny\n\
<protocol> : tcp | udp | icmp | ip\n\
<src IP>   : IPv4 address | any\n\
<dst IP>   : IPv4 adresa | any\n\
<srcPort>  : port number\n\
<dstPort>  : port number\n\
", stdout);
}

/* Send print command to the kernel module.
 * @return EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int print_rules()
{
  FILE *f;
  int ch;

  if ((f = fopen(PROCF_NAME, "r")) == NULL) {
    perror("fopen()");

    if (errno == ENOENT) {
      fprintf(stderr, "Error: Is the kernel module loaded? (try lsmod|grep pdsfw)\n");
    }

    return EXIT_FAILURE;
  }

  printf("id     action srcip           srcport dstip           dstport protocol\n");
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
 * @param   options structure containg program flags
 * @param   input file with rules
 * @return  EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int add_rules(struct cmdline_options *options, FILE *input)
{
  FILE *output;
  char line[256];

  eprintf(options->v, "> add_rules() called\n");

  if (options == NULL || input == NULL) {
    fprintf(stderr, "Internal Error: %s(NULL paramater)\n", __func__);
    return EXIT_FAILURE;
  }

  if ((output = fopen(PROCF_NAME, "w")) == NULL) {
    perror("fopen()");
    return EXIT_FAILURE;
  }

  while (fgets(line, sizeof(line), input) != NULL) {
    fprintf(output, line);

    /* write the line to the proc immediately so the proc write func in kernel module can read rules from the file line by line */
    if (fflush(output) == EOF) {
      perror("fflush()");
      return EXIT_FAILURE;
    }
    eprintf(options->v, "> (%dB written) line: %s", strlen(line), line);
  }

  if (fclose(output) == EOF) {
    perror("fclose()");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Write a single rule which was specified as a program argument after -a option
 * into the /proc file.
 *
 * @param   options  structure containg program flags
 * @param   rule  rule as a string
 * @return  EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int add_single_rule(struct cmdline_options *options, const char *rule)
{
  FILE *f;
  int ret;

  if (options == NULL || rule  == NULL) {
    fprintf(stderr, "Internal Error: %s(NULL paramater)\n", __func__);
    return EXIT_FAILURE;
  }

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

  eprintf(options->v, "> Buffer (%dB) was written to the file %s\n", ret, PROCF_NAME);

  if (fclose(f) == EOF) {
    fprintf(stderr, 
        "Error: There has been error while closing the file %s\n", PROCF_NAME);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* Construct and send a delete command to the kernel side.
 *
 * @param   num The id of the rule which will be deleted.
 * @return  EXIT_FAILURE on error, EXIT_SUCCESS otherwise
 */
int delete_rule(int num)
{
  FILE *f;

  if (num < ID_MIN || num > ID_MAX) {
    fprintf(stderr, "Error: %s() Invalid or out of range id: \'%s\'\n", __func__, optarg);
    return EXIT_FAILURE;
  }

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

/* Check the syntax of file where rules will be read from.
 * @options:  command line options including file name
 *
 * @return:   pointer to file with rules if the syntax is alright, NULL
 *            otherwise
 */
FILE *chech_file_syntax(struct cmdline_options *options)
{
  FILE *input;

  if (options == NULL) {
    fprintf(stderr, "Internal Error: NULL pointer to options struct\n");
    return NULL;
  }

  if ((input = fopen(options->file, "r")) == NULL) {
    perror("fopen()");
    return NULL;
  }

  yyin = input;
  if (yyparse() == EXIT_SUCCESS) {
    eprintf(options->v, "> syntax of rules in the file %s is OK\n", options->file);
  } else {
    return NULL;
  }

  /* get back to the begining of the file because yacc read it all */
  if (fseek(input, 0, SEEK_SET)) {
    fprintf(stderr, "Error: seeking to start of file");
    return NULL;
  }

  return input;
}

int main(int argc, char *argv[])
{
  char ch;
  FILE *input;
  int count = 0;
  /* buffer for single rule from command line (-a option) */
  char sr_buf[BUF_SIZE] = {[BUF_SIZE-1] = '\0'};
  struct cmdline_options options = {false, false, false, false, false, 0, NULL};

  program_name = argv[0];

  if (argc <= 1) {
    usage();
    return EXIT_SUCCESS;
  }

  while ((ch = getopt(argc, argv, "hpvaf:d:")) != EOF) {
    switch (ch) {
      case 'h':
        usage();
        return EXIT_SUCCESS;
      case 'v':
        options.v = true;
        break;
      case 'f':
        options.f = true;
        options.file = optarg;
        eprintf(options.v, "> firewall policy rules will be taken from the file %s\n", options.file);

        if ((input = chech_file_syntax(&options)) != NULL) {
          add_rules(&options, input);

          if (fclose(input) == EOF) {
            perror("fclose()");
            return EXIT_FAILURE;
          }
        } else {
          return EXIT_FAILURE;
        }
       break;
      case 'p':
        options.p = true;
        eprintf(options.v, "> Reading rules from %s\n", PROCF_NAME);
        print_rules();
        break;
      case 'd':
        options.d = true;
        if ((options.id = atoi(optarg)) == 0) {
          fprintf(stderr, "Error: Invalid or out of range id: \'%s\'\n", optarg);
          return EXIT_FAILURE;
        }
        eprintf(options.v, "> Removing rule with id=%d\n", options.id);
        delete_rule(options.id);
        break;
      case 'a':
        options.a= true;
        break;
      default:
        fprintf(stderr, "Unknown option: '%s'\n", optarg);
        return 1;
    }
  }

  argc -= optind;
  argv += optind;

  if (options.a) {
    eprintf(options.v, "> Addding a new rule\n");

    if (argc < 6) {
      fprintf(stderr, "Error: no rule specified\n");
      return EXIT_FAILURE;
    }

    for (count = 0; count < argc; count++) {
      strcat(sr_buf, argv[count]);
      strcat(sr_buf, " ");
    }

    YY_BUFFER_STATE buffer = yy_scan_string(sr_buf);
    if (yyparse() == EXIT_SUCCESS) {
      eprintf(options.v, "> syntax of the rule \'%s\' is OK\n", sr_buf);
      add_single_rule(&options, sr_buf);
    }
    yy_delete_buffer(buffer);

  } 
   
  return EXIT_SUCCESS;
}
