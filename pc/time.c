#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////
//
// Time functions.
//
// Uses Julian day number to work out the values returned by these functions
//
//


int time_jdn2(int d, int m, int y)
{
  int a1 = 367*y;
  int a2 = 275 * m;
  int a3 = a2 / 9;
  int a4 = 1729777;
  int a5 = (m-9)/7;
  int a6 = y+5001+a5;
  int a7 = 7 * a6;
  int a8 = a7 / 4;
  
  int jdn = (a1 - a8+a3+d+a4);
  
  return(jdn);
  
  //return(367*y-(7*(y+5001+(m-9)/7))/4+(275*m)/9+d+1729777);
}

int time_jdn(int d, int m, int y)
{
  return((1461 * (y + 4800 + (m - 14) / 12)) / 4 + (367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 - (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 + d - 32075);
}


  int time_dow(int d, int m, int y)
{
  return(1+(time_jdn(d, m, y) % 7));
}

