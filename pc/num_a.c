////////////////////////////////////////////////////////////////////////////////
//
// Num package
//
// BCD floating point
//
////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////

char *num_as_text(NOPL_FLOAT *n, char *text)
{
  static char line[40];
  static char part[20];
  
  if(n->sign)
    {
      strcpy(line, "-");
    }
  else
    {
      strcpy(line, "");
    }
  
  sprintf(part, "%d.%s", n->digits[0], text);
  strcat(line, part);
  
  for(int i=1; i<NUM_MAX_DIGITS; i++)
    {
      sprintf(part, "%d%s", n->digits[i], text);
      strcat(line, part);
    }

  sprintf(part, " E%d", (int)n->exponent);
  strcat(line, part);

  return(line);
}

//------------------------------------------------------------------------------

void dbq_num_exploded(char *text, NOPL_FLOAT *n)
{
  dbq("%s%s", text, num_as_text(n, " "));
}

void dbq_num(char *text, NOPL_FLOAT *n)
{
  dbq("%s%s", text, num_as_text(n, ""));
}

//------------------------------------------------------------------------------

void num_print(NOPL_FLOAT *n)
{
  printf("%s", num_as_text(n, ""));
}

////////////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------------

#define MAX_INDEX (NUM_MAX_DIGITS-1)

void digits_right(NOPL_FLOAT *n, int shift)
{
  
  for(int i = MAX_INDEX; i>0; i--)
    {
      n->digits[i] = n->digits[i-1];
    }

  for(int i=0; i<shift; i++)
    {
      n->digits[0] = 0;
    }
}

//------------------------------------------------------------------------------

// Make the exponent of n the same as that of r

void make_exp_same(NOPL_FLOAT *n, NOPL_FLOAT *r)
{
  n->exponent = r->exponent;
  digits_right(n, r->exponent - n->exponent);
}

//------------------------------------------------------------------------------


void num_shift_digits_right(NOPL_FLOAT *n)
{
  // Shift mantissa digits to the 'right'
  for(int i=NUM_MAX_DIGITS-1; i>=0; i--)
    {
      n->digits[i] = n->digits[i-1];
    }

  n->digits[0] = 0;
  (n->exponent)++;
}

//------------------------------------------------------------------------------

void num_shift_digits_left(NOPL_FLOAT *n)
{
  // Shift mantissa digits to the 'left'
  for(int i=0; i<NUM_MAX_DIGITS;  i++)
    {
      n->digits[i] = n->digits[i+1];
    }
  
  n->digits[NUM_MAX_DIGITS-1] = 0;
  (n->exponent)--;
}

//------------------------------------------------------------------------------
//

//------------------------------------------------------------------------------
//
// Propagate carries and shift exponents to get a non zero in digit[0]
//

void num_propagate_carry(NOPL_FLOAT *n)
{
  // Propagate carry
  for(int i= NUM_MAX_DIGITS-1; i>0; i--)
    {
      if( n->digits[i] >= 10 )
	{
	  n->digits[i] %= 10;
	  n->digits[i-1]++;
	}
    }
}

void num_normalise(NOPL_FLOAT *n)
{
  // If digits[0] is 0 then we need to shift left until it is non zero
  while( n->digits[0] == 0 )
    {
      num_shift_digits_left(n);
    }

  // If we have a carry in the LH digit then shift right until it has gone
  while( n->digits[0] >= 10 )
    {
      num_shift_digits_right(n);

      // Split the first digit into two
      n->digits[0] = 1;
      n->digits[1] %= 10;
    }
}

//------------------------------------------------------------------------------

void num_make_exponent(NOPL_FLOAT *n, int exp)
{
  // The num will be left unnormalised.
  // We shift mantissa to make the expoinent the required value. This may result
  // in all zeros in some cases.

  // There are two directions we may have to shift in.
  
  if( n->exponent < exp )
    {
      dbq("Right Exp:%d", exp);
      while(n->exponent < exp )
	{
	  num_shift_digits_right(n);
	}
    }
  else
    {
      dbq("Left Exp:%d", exp);
      
      while(n->exponent < exp )
	{
	  num_shift_digits_left(n);
	}
    }
}

//------------------------------------------------------------------------------
//
// Make mantissa a ten's complement

void num_mantissa_tens_compl(NOPL_FLOAT *n)
{
  dbq("");
  dbq_num("Before 10s compl", n);
  
  // Nine's complement
  for(int i=0; i<NUM_MAX_DIGITS; i++)
    {
      n->digits[i] = 9 - n->digits[i];
    }

  // Add 1 for the ten's complement
  n->digits[NUM_MAX_DIGITS-1]++;
  num_normalise(n);

  dbq_num("after 10's compl", n);
}

//------------------------------------------------------------------------------

void num_add_pos(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  // Adjust the smaller number so we have the same exponent. Shift the mantissa to line up.
  if( a->exponent > b->exponent)
    {
      num_make_exponent(b, a->exponent);
    }
  else
    {
      num_make_exponent(a, b->exponent);
    }
  
  // Add the mantissa digits
  for(int i=NUM_MAX_DIGITS-1; i>=0; i--)
    {
      r->digits[i] = a->digits[i] + b->digits[i]; 
    }

  r->exponent = a->exponent;
  r->sign = a->sign;

  dbq_num("Before normailse:", r);
  
  // Normalise
  num_normalise(r);
}

//------------------------------------------------------------------------------

void num_sub_pos(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  // Adjust the smaller number so we have the same exponent. Shift the mantissa to line up.
  if( a->exponent > b->exponent)
    {
      num_make_exponent(b, a->exponent);
    }
  else
    {
      num_make_exponent(a, b->exponent);
    }

  num_mantissa_tens_compl(a);
  
  // Add the mantissa digits
  for(int i=NUM_MAX_DIGITS-1; i>=0; i--)
    {
      r->digits[i] = a->digits[i] + b->digits[i]; 
    }
 
  r->exponent = a->exponent;
  r->sign = a->sign;
  
  // Normalise
  num_propagate_carry(r);

  // We will always have an overflow in the fiorst digit as we are adding a ten's
  // complement number
  r->digits[0] -= 10;
}

//------------------------------------------------------------------------------
void num_add(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  num_add_pos(a, b, r);
}

//------------------------------------------------------------------------------
void num_sub(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  num_sub_pos(a, b, r);
}

////////////////////////////////////////////////////////////////////////////////
//
// Comparisons
//
////////////////////////////////////////////////////////////////////////////////

int num_eq(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  if( a->exponent == b->exponent )
    {
      if( a->sign == b->sign )
	{
	  for(int i=0; i<NUM_MAX_DIGITS; i++)
	    {
	      if( a->digits[i] != b->digits[i] )
		{
		  return(0);
		}
	    }
	  return(1);
	}
    }
  
  return(0);
}

//------------------------------------------------------------------------------
//
// a> b?
//

int num_gt(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  // Different signs then positive is greater
  if( a->sign != b->sign )
    {
      if( NUM_POSITIVE(a) )
	{
	  return(1);
	}
      else
	{
	  return(0);
	}
    }

  // Signs the same, but negative numbers have opposite sense
  // so swap them
  if( NUM_NEGATIVE(a) )
    {
      NOPL_FLOAT *t;

      t = a;
      a = b;
      b = t;
    }
  
  if( a->exponent > b->exponent )
    {
      for(int i=0; i<NUM_MAX_DIGITS; i++)
	{
	  if( a->digits[i] > b->digits[i] )
	    {
	      return(1);
	    }
	}
    }
  
  return(0);
}

//------------------------------------------------------------------------------

int num_zero(NOPL_FLOAT *a)
{
  for(int i = 0; i< NUM_MAX_DIGITS; i++)
    {
      if( a->digits[i] != 0 )
	{
	  return(0);
	}
    }
  
  return(1);
}

int num_not(NOPL_FLOAT *a)
{
  if( num_zero(a) )
    {
      return(NOBJ_TRUE);
    }
  
  return(NOBJ_FALSE);
}

int num_true(NOPL_FLOAT *a)
{
  if( num_zero(a) == NOBJ_FALSE )
    {
      return(1);
    }
  
  return(0);
}


int num_and(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  if( num_true(a) && num_true(b) )
    {
      return(NOBJ_TRUE);
    }
  
  return(NOBJ_FALSE);
}

int num_or(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  if( num_true(a) || num_true(b) )
    {
      return(NOBJ_TRUE);
    }
  
  return(NOBJ_FALSE);
}

//------------------------------------------------------------------------------

int num_ne(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  return(!num_eq(a, b));
}

int num_lt(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  return(num_gt(b, a) && num_ne(a,b));
}

int num_gte(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  return(num_lt(b, a));
}

int num_lte(NOPL_FLOAT *a, NOPL_FLOAT *b)
{
  return(num_gt(b, a));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if 0

////////////////////////////////////////////////////////////////////////////////
//
// Conversion to and from C types
//
////////////////////////////////////////////////////////////////////////////////
//
// Psion datatypes are pointed to and stored in Psion format. Arrays are not
// converted as arrays, the elements have to be accessed and converted individually
//
// For now floats are converted to doubles, really a BCD data type should be used.
//

NOPL_INT psion_int(uint8_t *p)
{
  NOPL_INT x;

  x  = ((int)(*(p++)))<<8;
  x += *p;
  
  return(x);
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert a Psion float to a NOPL_FLOAT
//
////////////////////////////////////////////////////////////////////////////////

NOPL_FLOAT psion_float(uint8_t *p)
{
  NOPL_FLOAT nf;
  
  uint64_t m = 0;
  uint64_t mul = 1;
  
  for(int i = 0; i<6; i++)
    {
      int z = *(p++);
      double y = (double)((z & 0xF) + ((z>>4)*10));

      m +=  y * mul;
      mul *= 100.0;

#if DB_PSION_FLOAT
      printf("\n         m:%lu  mul:%lu z:%d y:%lf", m, mul, z, y);
      
#endif

    }

  nf.m = m;
  
  // Signed byte for exponent, binary
  int es = (int8_t)(*(p++));
  nf.e = es;
  
  nf.s = *p;
    
  return(nf);
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert a NOPL_FLOAT to a double
//
////////////////////////////////////////////////////////////////////////////////

double nopl_float_to_double(NOPL_FLOAT *nf)
{
  double r;

  r = nf->m;
  r *= pow(10, (nf->e)-11);
  if( (nf->s) == 0 )
    {
    }
  else
    {
      r = -r;
    }
  
  return(r);
}

////////////////////////////////////////////////////////////////////////////////
//
// Convert a NOPL_FLOAT to a string
//

char nopl_string[80];

char *nopl_float_str(NOPL_FLOAT *nf)
{
  if( nf->e <=12 )
    {
      sprintf(nopl_string, "%s%ldE%d", (nf->s)?"-":" ", nf->m, nf->e);
    }
  
  return(nopl_string);
}
#endif
