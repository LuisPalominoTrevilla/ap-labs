#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include "logger.h"

#define BUF_LEN (10 * (sizeof(struct inotify_event) + 1))

struct evt_info
{
  int cookie;
  char *name;
};

char *directory;

static void displayEvent(struct inotify_event *, struct evt_info *);
static int isDirectory(char *);
static char *joinPath(char *, char *);

int main(int argc, char **argv)
{
  int inotifyFd, wd;
  char *p;
  struct evt_info prevEvent = {-1, ""};
  struct inotify_event *event;
  ssize_t numread;
  char buf[BUF_LEN] __attribute__((aligned(8)));

  initLogger("stdout");
  if (argc < 2)
  {
    errorf("Usage: ./monitor.o [DIRNAME]\n");
    return 0;
  }

  // Validate argument is a directory
  directory = argv[1];
  if (!isDirectory(directory))
  {
    errorf("%s is not a directory\n", directory);
    return 0;
  }

  // Create inotify instance
  if ((inotifyFd = inotify_init()) == -1)
  {
    errorf("Unable to init inotify\n");
    return 0;
  }

  infof("Starting File/Directory Monitor on %s\n", directory);

  // Add watcher for selected events
  int allowedEvents = IN_CREATE | IN_DELETE | IN_MOVE;
  if ((wd = inotify_add_watch(inotifyFd, directory, allowedEvents)) == -1)
  {
    errorf("Unable to add watcher to directory\n");
    return 0;
  }

  for (;;)
  {
    numread = read(inotifyFd, buf, BUF_LEN);

    if (numread == 0)
    {
      warnf("inotify fd read returned 0\n");
    }

    if (numread == -1)
    {
      errorf("inotify fd read\n");
      return 0;
    }

    for (p = buf; p < buf + numread;)
    {
      event = (struct inotify_event *)p;
      displayEvent(event, &prevEvent);
      p += sizeof(struct inotify_event) + event->len;
    }
  }

  return 0;
}

static void displayEvent(struct inotify_event *e, struct evt_info *prevEvent)
{
  char *filePath = joinPath(directory, e->name);
  char *fileType = isDirectory(filePath) ? "Directory" : "File";
  free(filePath);

  if (e->mask & IN_CREATE)
  {
    infof("- [%s - Create] - %s\n", fileType, e->name);
  }
  if (e->mask & IN_DELETE)
  {
    infof("- [%s - Removal] - %s\n", fileType, e->name);
  }
  if (e->mask & IN_MOVED_TO && prevEvent->cookie == e->cookie)
  {
    infof("- [%s - Rename] - %s -> %s\n", fileType, prevEvent->name, e->name);
  }

  prevEvent->cookie = e->cookie;
  prevEvent->name = e->name;
}

static int isDirectory(char *path)
{
  struct stat path_stat;
  return stat(path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode);
}

static char *joinPath(char *path, char *path2)
{
  char *joined = malloc(strlen(path) + strlen(path2) + 2);
  strcpy(joined, path);
  strcat(joined, "/");
  strcat(joined, path2);
  return joined;
}
