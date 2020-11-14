#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "logger.h"

#define MAX_SIDES 2000

typedef long *mat;

int matSides;

mat readMatrix(char *filename);
long *getColumn(int col, mat matrix);
long *getRow(int row, mat matrix);
long dotProduct(long *vec1, long *vec2);

int main(int argc, char **argv)
{
  initLogger("stdout");
  mat matA = readMatrix("matA.dat");
  if (matA == NULL)
  {
    errorf("Could not read matrix A file\n");
    return 0;
  }

  mat matB = readMatrix("matB.dat");
  if (matB == NULL)
  {
    errorf("Could not read matrix B file\n");
    return 0;
  }

  free(matA);
  free(matB);
  return 0;
}

long dotProduct(long *vec1, long *vec2)
{
  long result = 0;
  for (int i = 0; i < matSides; i++)
  {
    result += vec1[i] * vec2[i];
  }

  return result;
}

long *getColumn(int col, mat matrix)
{
  if (col < 0 || col >= matSides)
    return NULL;

  long *matCol = malloc(sizeof(long) * matSides);
  if (matCol == NULL)
  {
    panicf("Unable to allocate buffer\n");
    exit(1);
  }
  for (int i = 0; i < matSides; i++)
    matCol[i] = matrix[i * matSides + col];

  return matCol;
}

long *getRow(int row, mat matrix)
{
  if (row < 0 || row >= matSides)
    return NULL;

  long *matRow = malloc(sizeof(long) * matSides);
  if (matRow == NULL)
  {
    panicf("Unable to allocate buffer\n");
    exit(1);
  }
  for (int n = 0; n < matSides; n++)
    matRow[n] = matrix[row * matSides + n];

  return matRow;
}

mat readMatrix(char *filename)
{
  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return NULL;

  size_t n = pow(MAX_SIDES, 2);
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
    matrix[i++] = strtol(*buf, NULL, 10);

  matSides = sqrt(i);
  fclose(f);
  return matrix;
}
