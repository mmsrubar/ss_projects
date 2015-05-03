#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  int m, k, n, i, j;
  int num;
  FILE *f1, *f2;

  if (argc < 4) {
    printf("%s m k n\n", argv[0]);
    return 1;
  }

  m = atoi(argv[1]);
  k = atoi(argv[2]);
  n = atoi(argv[3]);

  if ((f1 = fopen("mat1", "w")) == NULL) {
    perror("fopen");
    return -1;
  }

  if ((f2 = fopen("mat2", "w")) == NULL) {
    perror("fopen");
    return -1;
  }

  fprintf(f1, "%d\n", m);
  fprintf(f2, "%d\n", k);
    
  printf("Copy it to wolfram:\n");
  putchar('{');
  for (i = 0; i < m; i++) {
    putchar('{');
    for (j = 0; j < n; j++) {
      num = 0 + rand() / (RAND_MAX / (30 - 0 + 1) + 1);
      fprintf(f1, "%d ", num);
      (j == n-1) ? printf("%d", num) :printf("%d, ", num);
    }
    (i == m-1) ? putchar('}') : printf("}, ");
    fputc('\n', f1);
  }
  printf("} * ");

  putchar('{');
  for (i = 0; i < n; i++) {
    putchar('{');
    for (j = 0; j < k; j++) {
      num = 0 + rand() / (RAND_MAX / (30 - 0 + 1) + 1);
      fprintf(f2, "%d ", num);
      (j == k-1) ? printf("%d", num) :printf("%d, ", num);
    }
    fputc('\n', f2);
    (i == n-1) ? putchar('}') : printf("}, ");
  }
  printf("}\n");



  fclose(f1);
  fclose(f2);

  return 0;
}

