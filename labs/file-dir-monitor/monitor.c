#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "logger.h"

int main(int argc, char **argv)
{
  char *directory;
  initLogger("stdout");

  if (argc < 2)
  {
    errorf("Usage: ./monitor.o [DIRNAME]\n");
    return 0;
  }

  directory = argv[1];
  struct stat path_stat;
  if (stat(directory, &path_stat) != 0 || !S_ISDIR(path_stat.st_mode))
  {
    errorf("%s is not a directory\n", directory);
    return 0;
  }

  return 0;
}
