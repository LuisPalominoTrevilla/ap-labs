#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include "logger.h"

#define N 400

typedef long *mat;

char *RESULT_MATRIX_FILE = "result.dat";
int NUM_BUFFERS = 800;

pthread_mutex_t *mutexes;
long **buffers;

struct arg_struct
{
  int idx;
  int row;
  int col;
  mat matA;
  mat matB;
  mat result;
};

mat readMatrix(char *filename);
long *getColumn(int col, mat matrix);
long *getRow(int row, mat matrix);
long dotProduct(long *vec1, long *vec2);
mat multiply(mat matA, mat matB);
int saveResultMatrix(long *result);
int releaseLock(int lock);

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

  buffers = (long **)malloc(sizeof(long *) * NUM_BUFFERS);
  mutexes = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * NUM_BUFFERS);
  for (int i = 0; i < NUM_BUFFERS; i++)
  {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    mutexes[i] = mutex;
    pthread_mutex_init(&mutexes[i], NULL);
  }

  mat result = multiply(matA, matB);
  if (result == NULL)
  {
    errorf("Error while multiplying matrixes\n");
    free(matA);
    free(matB);
    free(mutexes);
    free(buffers);
    return 0;
  }

  if (saveResultMatrix(result) == -1)
    errorf("Could not save result matrix into file\n");

  free(result);
  free(matA);
  free(matB);
  free(mutexes);
  free(buffers);
  return 0;
}

int getLock(int priority)
{
  int maxLimit = NUM_BUFFERS - 1 + priority;
  for (int i = 0; i < maxLimit; i++)
  {
    if (pthread_mutex_trylock(&mutexes[i]) == 0)
      return i;
  }

  return -1;
}

int releaseLock(int lock)
{
  return pthread_mutex_unlock(&mutexes[lock]) != 0 ? -1 : 0;
}

static void *thread_operation(void *arguments)
{
  struct arg_struct *args = (struct arg_struct *)arguments;
  int l1, l2;

  while ((l1 = getLock(0)) == -1);
  while ((l2 = getLock(1)) == -1);

  buffers[l1] = getRow(args->row, args->matA);
  if (buffers[l1] == NULL)
  {
    panicf("Could not read row %d of matrix A\n", args->row);
    while (releaseLock(l1) != 0);
    while (releaseLock(l2) != 0);
    return (void *)-1;
  }

  buffers[l2] = getColumn(args->col, args->matB);
  if (buffers[l2] == NULL)
  {
    panicf("Could not read column %d of matrix B\n", args->col);
    free(buffers[l1]);
    while (releaseLock(l1) != 0);
    while (releaseLock(l2) != 0);
    return (void *)-1;
  }

  args->result[args->idx] = dotProduct(buffers[l1], buffers[l2]);
  free(buffers[l1]);
  free(buffers[l2]);
  free(arguments);
  while (releaseLock(l1) != 0);
  while (releaseLock(l2) != 0);
  return (void *)0;
}

mat multiply(mat matA, mat matB)
{
  size_t matSize = pow(N, 2);
  pthread_t *threads = malloc(sizeof(pthread_t) * matSize);
  mat result = malloc(sizeof(long) * matSize);
  for (int i = 0; i < N; i++)
  {
    for (int j = 0; j < N; j++)
    {
      int idx = i * N + j;
      struct arg_struct *threadArgs = (struct arg_struct *)malloc(sizeof(struct arg_struct));
      threadArgs->idx = idx;
      threadArgs->row = i;
      threadArgs->col = j;
      threadArgs->matA = matA;
      threadArgs->matB = matB;
      threadArgs->result = result;
      pthread_create(&threads[idx], NULL, thread_operation, threadArgs);
    }
  }

  int error = 0;
  for (int i = 0; i < matSize; i++)
  {
    if (pthread_join(threads[i], NULL) != 0)
      error = 1;
  }

  free(threads);
  if (error)
  {
    free(result);
    return NULL;
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
