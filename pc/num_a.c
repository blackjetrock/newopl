////////////////////////////////////////////////////////////////////////////////
//
// Num package
//
// BCD floating point
//
////////////////////////////////////////////////////////////////////////////////

#include "num_a.h"

void num_print(NUM *n)
{
  if(n->sign)
    {
      printf("-");
    }

  printf("%d.", n->digits[0]);

  for(int i=1; i<NUM_MAX_DIGITS; i++)
    {
      printf("%d", n->digits[i]);
    }
}

//------------------------------------------------------------------------------


void num_add(NUM *a, NUM *b, NUM *r)
{
  
}
