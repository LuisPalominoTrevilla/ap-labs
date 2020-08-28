#include <stdio.h>
#include <stdlib.h>

static char daytab[2][12] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
static char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* month_day function's prototype*/
void month_day(int year, int yearday, int *pmonth, int *pday);

int main(int argc, char **argv)
{
  int pmonth, pday, year, yearday;
  if (argc != 3)
  {
    printf("Usage: ./a.out <year> <yearday>\n");
    exit(1);
  }

  year = atoi(argv[1]);
  yearday = atoi(argv[2]);
  if (year < 0 || yearday < 1 || yearday > 366)
  {
    printf("Year or yearday parameter is incorrect\n");
    return 0;
  }

  month_day(year, yearday, &pmonth, &pday);
  printf("%s %02d, %d\n", months[pmonth], pday, year);
  return 0;
}

void month_day(int year, int yearday, int *pmonth, int *pday)
{
  int i, leap, acumDays;
  acumDays = 0;
  leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  for (i = 0; i < 12; i++)
  {
    if (daytab[leap][i] + acumDays >= yearday)
    {
      *pmonth = i;
      *pday = yearday - acumDays;
      return;
    }

    acumDays += daytab[leap][i];
  }
}
