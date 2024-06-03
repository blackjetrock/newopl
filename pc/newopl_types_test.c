////////////////////////////////////////////////////////////////////////////////
//
//
// Tests of the types funtions
//
//
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "newopl.h"
#include "nopl_obj.h"
#include "newopl_types.h"

uint8_t t1[]= {0x35, 0x0b};

void do_t1(void)
{
  int x = psion_int_to_int(t1);
  
  printf("\n%s: %s %d", __FUNCTION__, (x==13579)?"PASS":"FAIL", x);
}


typedef struct _FLOAT_TEST
  {
   uint8_t data[8];
   double result;
  } FLOAT_TEST;

FLOAT_TEST float_test[] =
  {
   {{0x00, 0x90, 0x78, 0x56, 0x34, 0x12, 2,  0x80},  -123.456789},
   {{0x00, 0x90, 0x78, 0x56, 0x34, 0x12, 2,  0x00},  123.456789},
   {{0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0,  0x00},    4.5},
   {{0x00, 0x00, 0x00, 0x00, 0x00, 0x10,  99, 0x00},   1.0e99},
   {{0x00, 0x00, 0x00, 0x00, 0x00, 0x10,  99, 0x80},  -1.0e99},
   {{0x00, 0x00, 0x00, 0x00, 0x00, 0x10, -99, 0x00},   1.0e-99},
   {{0x00, 0x00, 0x00, 0x00, 0x00, 0x10, -99, 0x80},  -1.0e-99},
  };

#define NUM_FLOAT_TEST (sizeof(float_test)/sizeof(FLOAT_TEST))

void do_t2(void)
{
  int pass = 1;
  double x;

  for(int t=0; t<NUM_FLOAT_TEST; t++)
    {
      x = psion_float_to_double(&(float_test[t].data[0]));

      if( fabs(x - float_test[t].result) < (1.0e-13) )
	{
	  printf("\n%s: PASS %f", __FUNCTION__, x);
	}
      else
	{
	  pass = 0;
	  double diff = (double)(x-float_test[t].result);
	  printf("\n%s: FAIL  %f (result) <> %f (desired value) difference: %f", __FUNCTION__, x, float_test[t].result, diff);
	}
    }
}

int main(void)
{
  do_t1();
  do_t2();

  printf("\n");
}
