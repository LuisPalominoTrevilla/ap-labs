#include <stdlib.h>

int mystrlen(char *str)
{
  int len = 0;
  while (*str != '\0')
  {
    len++;
    str++;
  }

  return len;
}

char *mystradd(char *origin, char *addition)
{
  int origLen = mystrlen(origin);
  int aditLen = mystrlen(addition);
  int n = origLen + aditLen;

  char *dest = (char *)malloc(n + 1);
  for (int i = 0; i < n; i++)
    dest[i] = i < origLen ? origin[i] : addition[i - origLen];

  dest[n] = '\0';
  return dest;
}

int mystrfind(char *origin, char *substr)
{
  int i = 0, j = 0, s = 0;
  int n = mystrlen(origin), slen = mystrlen(substr);
  if (n < slen)
    return -1;

  while (j < n)
  {
    if (origin[j] != substr[s])
    {
      j = ++i;
      s = 0;
      continue;
    }

    j++;
    s++;
    if (s == slen)
      return i;
  }

  return -1;
}

int mystrcmp(char *str1, char *str2)
{
  int n = mystrlen(str1);
  if (n != mystrlen(str2))
    return 0;

  for (int i = 0; i < n; i++)
  {
    if (str1[i] != str2[i])
      return 0;
  }

  return 1;
}
