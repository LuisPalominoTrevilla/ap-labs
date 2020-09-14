#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>

#define MAX_BUFF_SIZE 256
#define PACKAGE_REGEX "^\\[([^\\[])*\\][[:space:]]\\[ALPM\\][[:space:]](installed|reinstalled|upgraded|removed)[[:space:]]([^[:space:]]){1,}"
#define DATE_REGEX "^\\[([^\\[])*\\]"
#define ACTION_REGEX "(installed|reinstalled|upgraded|removed)[[:space:]]([^[:space:]]){1,}"
#define HASH_SIZE 101

struct packagesSummary
{
  int installed;
  int removed;
  int upgraded;
};

struct package
{
  struct package *next;
  char *name;
  char *installedOn;
  char *lastUpdated;
  char *removalDate;
  int numUpdates;
};

static struct package *hashtab[HASH_SIZE];

void analizeLog(char *logFile, char *report);
int processLogs(char *logFile);
int generateReport(char *reportFile);
void processPackageInfo(char *info, regex_t *dateRegex, regex_t *actionRegex);
int mygetline(char *lineptr, int maxbuf, int fd);
unsigned int hash(char *s);
struct package *lookup(char *s);
int insert(char *k, char *v);
void freeTable();

int main(int argc, char **argv)
{
  char *usageInfo = "Usage: ./pacman-analizer.o -input <file> -report <file>\n";
  char *logFile, *reportFile;

  if (argc < 5)
  {
    printf("%s", usageInfo);
    return 0;
  }

  if ((strcmp(argv[1], "-input") != 0 || strcmp(argv[3], "-report") != 0) &&
      (strcmp(argv[1], "-report") != 0 || strcmp(argv[3], "-input") != 0))
  {
    printf("%s", usageInfo);
    return 0;
  }

  logFile = strcmp(argv[1], "-input") == 0 ? argv[2] : argv[4];
  reportFile = strcmp(argv[1], "-report") == 0 ? argv[2] : argv[4];
  analizeLog(logFile, reportFile);
  freeTable();

  return 0;
}

void analizeLog(char *logFile, char *report)
{
  printf("Generating Report from: [%s] log file\n", logFile);

  if (processLogs(logFile) < 0)
    return;

  if (generateReport(report) < 0)
    return;

  printf("Report is generated at: [%s]\n", report);
}

int processLogs(char *logFile)
{
  char buf[MAX_BUFF_SIZE];
  int fd, len, err;
  regex_t packageRegex, dateRegex, actionRegex;

  if ((fd = open(logFile, O_RDONLY)) < 0)
  {
    printf("Could not open file [%s]\n", logFile);
    return -1;
  }

  if ((err = regcomp(&packageRegex, PACKAGE_REGEX, REG_EXTENDED | REG_ICASE | REG_NOSUB)) != 0)
  {
    printf("Could not compile package regex. Error code %d\n", err);
    return -1;
  }

  if ((err = regcomp(&dateRegex, DATE_REGEX, REG_EXTENDED | REG_ICASE)) != 0)
  {
    printf("Could not compile date regex. Error code %d\n", err);
    return -1;
  }

  if ((err = regcomp(&actionRegex, ACTION_REGEX, REG_EXTENDED | REG_ICASE)) != 0)
  {
    printf("Could not compile action regex. Error code %d\n", err);
    return -1;
  }

  while ((len = mygetline(buf, MAX_BUFF_SIZE, fd)) != -1)
  {
    if (regexec(&packageRegex, buf, 0, NULL, 0) == 0)
    {
      processPackageInfo(buf, &dateRegex, &actionRegex);
    }
  }

  close(fd);
  regfree(&packageRegex);
  regfree(&dateRegex);
  regfree(&actionRegex);
  return 0;
}

int generateReport(char *reportFile)
{
  int fd;
  struct package *pkg;
  struct packagesSummary summary = {0, 0, 0};

  unlink(reportFile);

  if ((fd = open(reportFile, O_WRONLY | O_CREAT, 0644)) < 0)
  {
    printf("Could not create report file\n");
    return -1;
  }

  for (int i = 0; i < HASH_SIZE; i++)
  {
    pkg = hashtab[i];
    while (pkg != NULL)
    {
      summary.installed++;
      if (pkg->lastUpdated != NULL)
        summary.upgraded++;
      if (pkg->removalDate != NULL)
        summary.removed++;
      pkg = pkg->next;
    }
  }

  dprintf(fd, "Pacman Packages Report\n----------------------\n");
  dprintf(fd, "- Installed packages\t: %d\n", summary.installed);
  dprintf(fd, "- Removed packages\t: %d\n", summary.removed);
  dprintf(fd, "- Upgraded packages\t: %d\n", summary.upgraded);
  dprintf(fd, "- Current installed\t: %d\n\n", summary.installed - summary.removed);

  dprintf(fd, "List of packages\n----------------\n");
  for (int i = 0; i < HASH_SIZE; i++)
  {
    pkg = hashtab[i];
    while (pkg != NULL)
    {
      dprintf(fd, "- Package Name\t\t\t: %s\n", pkg->name);
      dprintf(fd, "\t- Install date\t\t: %s\n", pkg->installedOn);
      dprintf(fd, "\t- Last update date\t: %s\n", pkg->lastUpdated == NULL ? "-" : pkg->lastUpdated);
      dprintf(fd, "\t- How many updates\t: %d\n", pkg->numUpdates);
      dprintf(fd, "\t- Removal date\t\t: %s\n", pkg->removalDate == NULL ? "-" : pkg->removalDate);
      pkg = pkg->next;
    }
  }

  close(fd);
  return 0;
}

void processPackageInfo(char *info, regex_t *dateRegex, regex_t *actionRegex)
{
  regmatch_t dateMatches[1], actionMatches[1];
  int packageOffset, dateLength, actionLength, packageNameLength;
  char *packagePos, *date, *action, *packageName;
  struct package *pkg;

  if ((regexec(dateRegex, info, 1, dateMatches, 0) | regexec(actionRegex, info, 1, actionMatches, 0)) != 0)
  {
    printf("Unexpected error while executing regex\n");
    return;
  }

  packagePos = strchr(info + actionMatches[0].rm_so, ' ');
  packageOffset = (int)((long long)packagePos - (long long)info);

  dateLength = (int)(dateMatches[0].rm_eo - dateMatches[0].rm_so - 2);
  actionLength = (int)(packageOffset - actionMatches[0].rm_so);
  packageNameLength = (int)(actionMatches[0].rm_eo - packageOffset - 1);

  date = (char *)malloc(sizeof(char) * (dateLength + 1));
  action = (char *)malloc(sizeof(char) * (actionLength + 1));
  packageName = (char *)malloc(sizeof(char) * (packageNameLength + 1));

  sprintf(date, "%.*s", dateLength, info + dateMatches[0].rm_so + 1);
  sprintf(action, "%.*s", actionLength, info + actionMatches[0].rm_so);
  sprintf(packageName, "%.*s", packageNameLength, packagePos + 1);

  if ((pkg = lookup(packageName)) == NULL)
  {
    if (strcmp(action, "installed") != 0 && strcmp(action, "reinstalled") != 0)
      printf("Unexpected error. Found %s package log but package had not been previously installed\n", action);
    if (insert(packageName, date) < 0)
      printf("Unexpected error. Could not insert package into hash table\n");
  }
  else if (strcmp(action, "upgraded") == 0)
  {
    // Increment upgrade counter and set last updated
    pkg->numUpdates++;
    pkg->lastUpdated = strdup(date);
  }
  else if (strcmp(action, "removed") == 0)
  {
    // Set removal date
    pkg->removalDate = strdup(date);
  }
  else if (strcmp(action, "installed") == 0 || strcmp(action, "reinstalled") == 0)
  {
    // TODO: Figure out what to do in this scenario
    // printf("Raro, se volvió a instalar %s con fecha de delete en %s\n", pkg->name, pkg->removalDate);
  }

  free(date);
  free(action);
  free(packageName);
}

int mygetline(char *lineptr, int maxbuf, int fd)
{
  int rd, i = 0;
  if ((rd = read(fd, lineptr, maxbuf)) == 0)
  {
    lineptr[i] = '\0';
    return -1;
  }

  while (i < rd && lineptr[i] != '\n' && lineptr[i] != EOF)
    i++;

  if (i < rd)
    lineptr[++i] = '\0';

  lseek(fd, -1 * (rd - i), SEEK_CUR);
  return i;
}

// Hash function referenced from the C Programming Language book
unsigned int hash(char *s)
{
  unsigned hashval;

  for (hashval = 0; *s != '\0'; s++)
    hashval = *s + 31 * hashval;

  return hashval % HASH_SIZE;
}

struct package *lookup(char *s)
{
  struct package *pkg;

  for (pkg = hashtab[hash(s)]; pkg != NULL; pkg = pkg->next)
  {
    if (strcmp(s, pkg->name) == 0)
      return pkg;
  }

  return NULL;
}

int insert(char *k, char *v)
{
  struct package *pkg;
  unsigned hashval;

  if ((pkg = lookup(k)) == NULL)
  {
    pkg = (struct package *)malloc(sizeof(*pkg));
    if (pkg == NULL || (pkg->name = strdup(k)) == NULL)
      return -1;

    hashval = hash(k);
    pkg->next = hashtab[hashval];
    hashtab[hashval] = pkg;
    pkg->lastUpdated = NULL;
    pkg->numUpdates = 0;
    pkg->removalDate = NULL;
  }
  else
    return -1;

  if ((pkg->installedOn = strdup(v)) == NULL)
    return -1;

  return 0;
}

void freeTable()
{
  struct package *pkg;
  struct package *next;

  for (int i = 0; i < HASH_SIZE; i++)
  {
    pkg = hashtab[i];
    while (pkg != NULL)
    {
      next = pkg->next;
      if (pkg->name != NULL)
        free(pkg->name);
      if (pkg->installedOn != NULL)
        free(pkg->installedOn);
      if (pkg->lastUpdated != NULL)
        free(pkg->lastUpdated);
      if (pkg->removalDate != NULL)
        free(pkg->removalDate);
      free(pkg);
      pkg = next;
    }
    hashtab[i] = NULL;
  }
}
