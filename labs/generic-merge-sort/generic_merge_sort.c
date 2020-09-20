#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINES 500

void mergeSort(void **lineptr, int left, int right, int (*comp)(void *, void *));
void merge(void **lineptr, int left, int mid, int right, int (*comp)(void *, void *));
int numcmp(const char *s1, const char *s2);

int main(int argc, char **argv)
{
  FILE *f;
  size_t len = 0;
  ssize_t read;
  char *buf = NULL;
  char *lineptr[MAXLINES];
  int lines = 0;
  int numeric = 0;
  char *file;

  if (argc < 2 || argc > 3)
  {
    printf("Usage: ./generic_merge_sort.o [-n] file.txt\n");
    return 0;
  }

  if (strcmp(argv[1], "-n") == 0)
  {
    numeric = 1;
    if (argc != 3)
    {
      printf("Missing file argument\n");
      return 0;
    }

    file = argv[2];
  }
  else
  {
    file = argv[1];
  }

  if ((f = fopen(file, "r")) == NULL)
  {
    printf("File not found: [%s]\n", file);
    return 0;
  }

  while ((read = getline(&buf, &len, f)) != -1)
  {
    lineptr[lines] = (char *)malloc(sizeof(char) * read);
    strcpy(lineptr[lines++], buf);
    if (lineptr[lines - 1][read - 1] == '\n')
      lineptr[lines - 1][read - 1] = '\0';
  }

  mergeSort((void **)lineptr, 0, lines - 1, (int (*)(void *, void *))(numeric ? numcmp : strcmp));

  fclose(f);
  if (buf)
    free(buf);
  for (int j = 0; j < lines; j++)
  {
    printf("%s\n", lineptr[j]);
    free(lineptr[j]);
  }
  return 0;
}

void mergeSort(void **lineptr, int left, int right, int (*comp)(void *, void *))
{
  int mid = (left + right) / 2;
  if (left >= right)
    return;
  mergeSort(lineptr, left, mid, comp);
  mergeSort(lineptr, mid + 1, right, comp);
  merge(lineptr, left, mid, right, comp);
}

void merge(void **lineptr, int left, int mid, int right, int (*comp)(void *, void *))
{
  int i = left, j = mid + 1, k = 0, n = right - left + 1;
  void **tmp = (void *)malloc(sizeof(void *) * n);
  memcpy(tmp, lineptr + left, n);

  while (i <= mid && j <= right)
  {
    tmp[k++] = (*comp)(lineptr[i], lineptr[j]) < 0 ? lineptr[i++] : lineptr[j++];
  }

  while (i <= mid)
    tmp[k++] = lineptr[i++];

  while (j <= right)
    tmp[k++] = lineptr[j++];

  for (k = 0; k < n; k++)
    lineptr[k + left] = tmp[k];

  free(tmp);
}

int numcmp(const char *s1, const char *s2)
{
  double v1, v2;

  v1 = atof(s1);
  v2 = atof(s2);
  if (v1 < v2)
    return -1;
  else if (v1 > v2)
    return 1;
  else
    return 0;
}
