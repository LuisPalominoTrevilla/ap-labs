#include <stdio.h>

#define MAX_ARRAY_LENGTH 100

void reverse(int, char *);

int main()
{
  int i = 0;
  char c;
  char line[MAX_ARRAY_LENGTH];

  while ((c = getchar()) != EOF)
  {
    if (c == '\n')
    {
      line[i] = '\0';
      reverse(i, line);
      printf("%s\n", line);
      i = 0;
      continue;
    }

    line[i++] = c;
  }

  return 0;
}

void reverse(int n, char *arr)
{
  char tmp;

  for (int i = 0; i < n / 2; i++)
  {
    tmp = arr[i];
    arr[i] = arr[n - i - 1];
    arr[n - i - 1] = tmp;
  }
}
