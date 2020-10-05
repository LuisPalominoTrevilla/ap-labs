#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/inotify.h>
#include "logger.h"

#define BUF_LEN (10 * (sizeof(struct inotify_event) + 1))

struct evt_info
{
  int cookie;
  char *name;
};

struct watched_dir
{
  int wd;
  char *dir;
  struct watched_dir *next;
};

struct watched_dir *watchDirectories;
char *rootDir;

static void displayEvent(struct inotify_event *, struct evt_info *);

static int isDirectory(char *);
static char *joinPaths(char *[], int);

static int addWatchDir(char *, char *, int);
static void removeWatchDir(int);
static struct watched_dir *findWatchDir(int);
static void freeWatchDirs();

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
  rootDir = argv[1];
  if (!isDirectory(rootDir))
  {
    errorf("%s is not a directory\n", rootDir);
    return 0;
  }

  // Create inotify instance
  if ((inotifyFd = inotify_init()) == -1)
  {
    errorf("Unable to init inotify\n");
    return 0;
  }

  infof("Starting File/Directory Monitor on %s\n", rootDir);

  // Add watcher to root directory
  if (addWatchDir(rootDir, ".", inotifyFd) == -1)
  {
    errorf("Unable to add watcher to directory\n");
    return 0;
  }

  // Free data structure when program ends
  signal(SIGINT, freeWatchDirs);

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
      freeWatchDirs();
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
  struct watched_dir *directory;
  if ((directory = findWatchDir(e->wd)) == NULL)
  {
    panicf("Received an event but could not retrieve watched directory");
    exit(1);
  }

  char *paths[3] = {rootDir, directory->dir, e->name};
  char *filePath = joinPaths(paths, 3);
  warnf("File evt is %s\n", filePath);
  char *fileType = isDirectory(filePath) ? "Directory" : "File";
  free(filePath);

  if (e->mask & IN_CREATE)
  {
    infof("- [%s - Create] - %s\n", fileType, e->name);
  }
  if (e->mask & IN_DELETE)
  {
    infof("- [File/Directory - Removal] - %s\n", e->name);
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

static char *joinPaths(char *paths[], int n)
{
  int size = 0;
  int i;
  for (i = 0; i < n; i++)
  {
    size += strlen(paths[i]);
  }

  char *joined = malloc(size + n);
  if (size == 0)
    return joined;

  strcpy(joined, paths[0]);
  for (i = 1; i < n; i++)
  {
    strcat(joined, "/");
    strcat(joined, paths[i]);
  }

  return joined;
}

static int addWatchDir(char *dir, char *path, int inotifyFd)
{
  int allowedEvents = IN_CREATE | IN_DELETE | IN_MOVE;
  int wd;
  if ((wd = inotify_add_watch(inotifyFd, dir, allowedEvents)) == -1)
    return -1;

  struct watched_dir *newWatchDir = malloc(sizeof(struct watched_dir));
  newWatchDir->dir = path;
  newWatchDir->wd = wd;
  if (watchDirectories != NULL)
  {
    newWatchDir->next = watchDirectories;
  }

  watchDirectories = newWatchDir;
  return 0;
}

static void removeWatchDir(int wd)
{
  struct watched_dir *curr = watchDirectories;
  struct watched_dir *prev;
  while (curr != NULL)
  {
    if (curr->wd == wd)
    {
      if (prev != NULL)
        prev->next = curr->next;
      else
        watchDirectories = curr->next;

      free(curr);
      return;
    }

    prev = curr;
    curr = curr->next;
  }
}

static struct watched_dir *findWatchDir(int wd)
{
  struct watched_dir *curr = watchDirectories;
  while (curr != NULL)
  {
    if (curr->wd == wd)
      return curr;

    curr = curr->next;
  }

  return NULL;
}

static void freeWatchDirs()
{
  struct watched_dir *curr = watchDirectories;
  while (curr != NULL)
  {
    warnf("Freed directory %s\n", curr->dir);
    struct watched_dir *next = curr->next;
    free(curr);
    curr = next;
  }

  exit(0);
}
