#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

#define ROWS 2000l
#define COLS 2000l

typedef long *mat;

mat readMatrix(char *filename);

int main(int argc, char **argv)
{
  initLogger("stdout");
  mat matA = readMatrix("matA.dat");
  if (matA == NULL)
  {
    errorf("Could not read matrix A file\n");
    return 0;
  }

  infof("Read and got %ld %ld\n", matA[0], matA[1]);

  free(matA);
  return 0;
}

mat readMatrix(char *filename)
{
  FILE *f = fopen(filename, "r");
  if (f == NULL)
  {
    return NULL;
  }

  size_t n = ROWS * COLS;
  mat matrix = (mat)malloc(sizeof(long) * n);
  if (matrix == NULL)
  {
    panicf("Unable to allocate buffer\n");
    exit(1);
  }

  char *buf[30];
  size_t len = 30;
  int i = 0;
  while (getline(buf, &len, f) != -1 && i < n)
  {
    matrix[i++] = strtol(*buf, NULL, 10);
  }

  fclose(f);
  return matrix;
}
