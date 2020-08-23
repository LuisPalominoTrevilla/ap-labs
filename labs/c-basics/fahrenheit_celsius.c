#include <stdio.h>
#include <stdlib.h>

/* print Fahrenheit-Celsius table */

float toCelsius(int);
void printConversion(int);

int main(int argc, char **argv)
{
  if (argc == 2) {
    printConversion(atoi(argv[1]));
  } else if (argc == 4) {
    int step = atoi(argv[3]);
    if (step <= 0) {
      printf("Step must be greater than 0\n");
      exit(1);
    }

    for (int fahr = atoi(argv[1]); fahr <= atoi(argv[2]); fahr = fahr + step) {
      printConversion(fahr);
    }
  } else {
    printf("Expected parameter(s) for simple conversion or range conversion\n");
    exit(1);
  }

  return 0;
}

void printConversion(int fahr) {
  printf("Fahrenheit: %3d, Celcius: %6.1f\n", fahr, toCelsius(fahr));
}

float toCelsius(int fahr)
{
  return (5.0 / 9.0) * (fahr - 32);
}
