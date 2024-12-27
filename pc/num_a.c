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

#define MAX_INDEX (NUM_MAX_DIGITS-1)
void digits_right(NUM *n, int n)
{
  
  for(int i = MAX_INDEX; i>0; i--)
    {
      n->digits[i] = n->digits[i-1];
    }

  for(int i=0; i<n; i++)
    {
      n->digits = 0;
    }
}

//------------------------------------------------------------------------------

// Make the exponent of n the same as that of r

void make_exp_same(NUM *n, NUM *r)
{
  n->exponent = r->exponent;
  digits_right(n, r->exponent - n->exponent);
}

//------------------------------------------------------------------------------


void num_add(NUM *a, NUM *b, NUM *r)
{
  NUM a1;
  NUM b1;

  b1 = *b;
  a1 = *a;	

  // Un-normalise to the larger number
  if( a->exponent > b->exponent )
    {

      make_exp_same(&b1, &a1);
    }
  else
    {
      make_exp_same(&a1, &b1);
    }
}





