#include <stdio.h>
#include <stdlib.h>

int mystrlen(char *str);
char *mystradd(char *origin, char *addition);
int mystrfind(char *origin, char *substr);
int mystrcmp(char *str1, char *str2);

int main(int argc, char **argv)
{
  char *mode, *p1, *p2;
  if (argc != 4)
  {
    printf("Error: Wrong number of arguments received\n");
    return 0;
  }

  mode = argv[1];
  p1 = argv[2];
  p2 = argv[3];
  if (mystrcmp(mode, "-add"))
  {
    char *newStr = mystradd(p1, p2);
    printf("Initial Length \t: %d\n", mystrlen(p1));
    printf("New String \t: %s\n", newStr);
    printf("New Length \t: %d\n", mystrlen(newStr));
    free(newStr);
  }
  else if (mystrcmp(mode, "-find"))
  {
    int pos = mystrfind(p1, p2);
    if (pos != -1)
    {
      printf("[%s] string was found at [%d] position\n", p2, pos);
    }
    else
    {
      printf("[%s] string was not found\n", p2);
    }
  }
  else
  {
    printf("Option %s is not supported. Try -add or -find instead.\n", mode);
  }

  return 0;
}
