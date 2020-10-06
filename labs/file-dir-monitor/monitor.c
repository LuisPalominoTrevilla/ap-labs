#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/inotify.h>
#include "logger.h"
#include <ftw.h>

#define BUF_LEN (10 * (sizeof(struct inotify_event) + 1))

struct evt_info
{
  int cookie;
  char *name;
};

struct watched_dir
{
  int wd;
  int level;
  char *fpath;
  struct watched_dir *next;
};

int inotifyFd;
struct watched_dir *watchDirectories;

static void displayEvent(struct inotify_event *e, struct evt_info *prevEvent);

static int monitorDirTree(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);
static int isDirectory(char *path);
static char *joinPath(char *basePath, char *filename);

static int addWatchDir(const char *dir, int level);
static struct watched_dir *findWatchDir(int wd);
static void freeWatchDirs();

int main(int argc, char **argv)
{
  char *p, *rootDir;
  struct evt_info prevEvent = {-1, NULL};
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
    panicf("Unable to init inotify\n");
    return 0;
  }

  infof("Starting File/Directory Monitor on %s\n", rootDir);
  if (nftw(rootDir, monitorDirTree, 20, FTW_ACTIONRETVAL) == -1)
  {
    panicf("Unable to look for directories inside root directory\n");
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
    return;
  }

  char *filePath = joinPath(directory->fpath, e->name);
  int isDir = isDirectory(filePath);
  char *fileType = isDir ? "Directory" : "File";

  if (e->mask & IN_CREATE)
  {
    if (directory->level == 0 && isDir && addWatchDir(filePath, 1) == -1)
    {
      panicf("Unable to add watcher to created directory\n");
      freeWatchDirs();
      free(filePath);
      exit(1);
    }

    infof("- [%s - Create] - %s\n", fileType, filePath);
  }
  if (e->mask & IN_DELETE)
  {
    infof("- [File - Removal] - %s\n", filePath);
  }
  if (e->mask & IN_MOVED_TO && directory->level == 0 && isDir && addWatchDir(filePath, 1) == -1)
  {
    panicf("Unable to add watcher to moved directory\n");
    freeWatchDirs();
    free(filePath);
    exit(1);
  }
  if (e->mask & IN_MOVED_TO && prevEvent->cookie == e->cookie)
  {
    infof("- [%s - Rename] - %s -> %s\n", fileType, prevEvent->name, filePath);
  }

  if (prevEvent->name != NULL)
    free(prevEvent->name);
  prevEvent->cookie = e->cookie;
  prevEvent->name = filePath;
}

static int monitorDirTree(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
  if (tflag & FTW_D && addWatchDir(fpath, ftwbuf->level) == -1)
  {
    panicf("Unable to add watcher to directory\n");
    return -1;
  }

  return ftwbuf->level == 0 ? FTW_CONTINUE : FTW_SKIP_SUBTREE;
}

static int isDirectory(char *path)
{
  struct stat path_stat;
  return stat(path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode);
}

static char *joinPath(char *basePath, char *filename)
{
  char *joined = malloc(strlen(basePath) + strlen(filename) + 2);
  strcpy(joined, basePath);
  strcat(joined, "/");
  strcat(joined, filename);
  return joined;
}

static int addWatchDir(const char *dir, int level)
{
  int allowedEvents = IN_CREATE | IN_DELETE | IN_MOVE;
  int wd;
  if ((wd = inotify_add_watch(inotifyFd, dir, allowedEvents)) == -1)
    return -1;

  struct watched_dir *newWatchDir = malloc(sizeof(struct watched_dir));
  newWatchDir->level = level;
  newWatchDir->wd = wd;
  newWatchDir->fpath = malloc(strlen(dir) + 1);
  strcpy(newWatchDir->fpath, dir);
  newWatchDir->next = watchDirectories;
  watchDirectories = newWatchDir;
  return 0;
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
    struct watched_dir *next = curr->next;
    free(curr->fpath);
    free(curr);
    curr = next;
  }

  exit(0);
}
