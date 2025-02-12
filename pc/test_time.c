#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include "nopl.h"

#include "time.h"

int main(void)
{
  int d = 12;
  int m = 2;
  int y = 2025;

  d = 1; m = 1; y = 2000;
  d = 12; m = 2; y = 2025;
  
  printf("\n%d", 2451545);
  printf("\n%d", 2451545 % 7);
  printf("\njdn(%d,%d,%d)=%d\n", d, m , y, time_jdn(d, m, y));
  printf("\ndow(%d,%d,%d)=%d\n", d, m , y, time_dow(d, m, y));
}
