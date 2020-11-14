#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include "logger.h"

#define STDOUT 0
#define SYSLOG 1

#define BLACK 0
#define RED 1
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define WHITE 7

static int logOutput = STDOUT;

int initLogger(char *logType)
{
  if (strcmp(logType, "stdout") == 0)
  {
    logOutput = STDOUT;
  }
  else if (strcmp(logType, "syslog") == 0)
  {
    openlog("Custom Logger -", LOG_CONS, LOG_USER);
    logOutput = SYSLOG;
  }
  else
  {
    printf("Invalid log type %s. Defaulting to stdout\n", logType);
  }

  printf("Initializing Logger on: %s\n", logType);
  return 0;
}

void __colorizeLog(int fg, int bg, int type, const char *title, const char *text, va_list args)
{
  if (logOutput == STDOUT)
  {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    printf("\x1b[%d;%dm [%d-%d-%dT%d:%d:%d] %s: \x1b[0m ", fg + 30, bg + 40, timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, title);
    vprintf(text, args);
  }
  else
  {
    vsyslog(type, text, args);
  }
}

int infof(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  __colorizeLog(WHITE, BLUE, LOG_INFO, "INFO", format, args);
  va_end(args);
  return 0;
}

int warnf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  __colorizeLog(BLACK, YELLOW, LOG_WARNING, "WARN", format, args);
  va_end(args);
  return 0;
}

int errorf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  __colorizeLog(WHITE, RED, LOG_ERR, "ERROR", format, args);
  va_end(args);
  return 0;
}

int panicf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  __colorizeLog(WHITE, MAGENTA, LOG_EMERG, "PANIC", format, args);
  va_end(args);
  return 0;
}
