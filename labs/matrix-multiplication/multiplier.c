#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "logger.h"

#define N 3

typedef long *mat;

char *RESULT_MATRIX_FILE = "result.dat";

mat readMatrix(char *filename);
long *getColumn(int col, mat matrix);
long *getRow(int row, mat matrix);
long dotProduct(long *vec1, long *vec2);
mat multiply(mat matA, mat matB);
int saveResultMatrix(long *result);

int main(int argc, char **argv)
{
  initLogger("stdout");
  mat matA = readMatrix("matA.dat");
  if (matA == NULL)
  {
    errorf("Could not create matrix A\n");
    return 0;
  }

  mat matB = readMatrix("matB.dat");
  if (matB == NULL)
  {
    errorf("Could not create matrix B\n");
    free(matA);
    return 0;
  }

  mat result = multiply(matA, matB);
  if (saveResultMatrix(result) == -1)
    errorf("Could not save result matrix into file\n");

  free(result);
  free(matA);
  free(matB);
  return 0;
}

mat multiply(mat matA, mat matB)
{
  mat result = malloc(sizeof(long) * pow(N, 2));
  for (int i = 0; i < N; i++)
  {
    for (int j = 0; j < N; j++)
    {
      long *rowA = getRow(i, matA);
      if (rowA == NULL)
      {
        panicf("Could not read row %d of matrix A\n", i);
        free(result);
        exit(1);
      }

      long *colB = getColumn(j, matB);
      if (colB == NULL)
      {
        panicf("Could not read column %d of matrix B\n", j);
        free(rowA);
        free(result);
        exit(1);
      }

      int idx = i * N + j;
      result[idx] = dotProduct(rowA, colB);
      free(rowA);
      free(colB);
    }
  }

  return result;
}

int saveResultMatrix(mat result)
{
  FILE *f = fopen(RESULT_MATRIX_FILE, "w");
  if (f == NULL)
    return -1;

  for (int i = 0; i < pow(N, 2); i++)
  {
    fprintf(f, "%ld\n", result[i]);
  }

  infof("Saved result to %s\n", RESULT_MATRIX_FILE);
  fclose(f);
  return 0;
}

long dotProduct(long *vec1, long *vec2)
{
  long result = 0;
  for (int i = 0; i < N; i++)
  {
    result += vec1[i] * vec2[i];
  }

  return result;
}

long *getColumn(int col, mat matrix)
{
  if (col < 0 || col >= N)
    return NULL;

  long *matCol = malloc(sizeof(long) * N);
  if (matCol == NULL)
  {
    panicf("Unable to allocate buffer\n");
    exit(1);
  }
  for (int i = 0; i < N; i++)
    matCol[i] = matrix[i * N + col];

  return matCol;
}

long *getRow(int row, mat matrix)
{
  if (row < 0 || row >= N)
    return NULL;

  long *matRow = malloc(sizeof(long) * N);
  if (matRow == NULL)
  {
    panicf("Unable to allocate buffer\n");
    exit(1);
  }
  for (int n = 0; n < N; n++)
    matRow[n] = matrix[row * N + n];

  return matRow;
}

mat readMatrix(char *filename)
{
  FILE *f = fopen(filename, "r");
  if (f == NULL)
    return NULL;

  size_t matSize = pow(N, 2);
  mat matrix = (mat)malloc(sizeof(long) * matSize);
  if (matrix == NULL)
  {
    panicf("Unable to allocate buffer\n");
    exit(1);
  }

  char *buf[30];
  size_t len = 30;
  int i = 0;
  while (getline(buf, &len, f) != -1 && i < matSize)
    matrix[i++] = strtol(*buf, NULL, 10);

  if (i != matSize)
  {
    errorf("Matrix in file %s has %ld elements, but expected %ld\n", filename, i, matSize);
    fclose(f);
    free(matrix);
    return NULL;
  }

  fclose(f);
  return matrix;
}
