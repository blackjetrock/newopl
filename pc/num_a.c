////////////////////////////////////////////////////////////////////////////////
//
// Num package
//
// BCD floating point
//
////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
//#include <double.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////

char *num_as_text(NOPL_FLOAT *n, char *text)
{
  static char line[40];
  static char part[20];
  
  if(n->sign)
    {
      sprintf(line, " - (%02X)", (n->sign & 0xFF));
    }
  else
    {
      sprintf(line, " + (%02X)", n->sign);
    }
  
  sprintf(part, " %d . %s", n->digits[0], text);
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
       
void dbq_num_exploded_f(const char *caller, char *text, NOPL_FLOAT *n)
{
  dbpfq(caller, text, num_as_text(n, " "));
}

void dbq_num_f(const char *caller, char *text, NOPL_FLOAT *n)
{
  dbpfq(caller, text, num_as_text(n, ""));
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
  dbq_num_exploded("%s Entry:", n);
  
  // If digits[0] is 0 then we need to shift left until it is non zero
  while( n->digits[0] == 0 )
    {
      num_shift_digits_left(n);
    }

  dbq_num_exploded("%s After shift", n);
  
  // If we have a carry in the LH digit then shift right until it has gone
  while( n->digits[0] >= 10 )
    {
      num_shift_digits_right(n);

      // Split the first digit into two
      n->digits[0] = 1;
      n->digits[1] %= 10;
    }
  
    dbq_num_exploded("%s After first dig split:", n);
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
  dbq_num_exploded("%s Before 10s compl", n);

  // Nine's complement
  for(int i=0; i<NUM_MAX_DIGITS; i++)
    {
      n->digits[i] = 9 - n->digits[i];
    }

  // Add 1 for the ten's complement
  n->digits[NUM_MAX_DIGITS-1]++;

  num_propagate_carry(n);
  num_normalise(n);

  dbq_num_exploded("%s after 10's compl", n);
}

//------------------------------------------------------------------------------
//
// Add positive floats
//
// Ignores signs
//

void num_add_pos(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  dbq("ADD POS");
  dbq_num("%s a:", a);
  dbq_num("%s b:", b);
  
  // Adjust the smaller number so we have the same exponent. Shift the mantissa to line up.
  if( a->exponent > b->exponent)
    {
      num_make_exponent(b, a->exponent);
    }
  else
    {
      num_make_exponent(a, b->exponent);
    }

  dbq_num("%s After exp adj a:", a);
  dbq_num("%s After exp adj b:", b);
  
  // Add the mantissa digits
  for(int i=NUM_MAX_DIGITS-1; i>=0; i--)
    {
      r->digits[i] = a->digits[i] + b->digits[i]; 
    }

  dbq_num("%s After digit sub r:", r);
  
  r->exponent = a->exponent;

  dbq_num("%s After digit asgn r:", r);
  
  dbq_num("%s Before normailse:", r);
  
  // Normalise
  num_normalise(r);
  dbq_num("%s After normailse:", r);
}

//------------------------------------------------------------------------------
//
// Subtract positive floats
//
// Ignores signs
//

void num_sub_pos(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  dbq("SUB POS");
  dbq_num("%s a:", a);
  dbq_num("%s b:", b);

  // Initialise sign
  r->sign = a->sign;
  
  // Adjust the smaller number so we have the same exponent. Shift the mantissa to line up.
  if( a->exponent > b->exponent)
    {
      num_make_exponent(b, a->exponent);
    }
  else
    {
      num_make_exponent(a, b->exponent);
    }

  num_mantissa_tens_compl(b);
  
  // Add the mantissa digits
  for(int i=NUM_MAX_DIGITS-1; i>=0; i--)
    {
      r->digits[i] = a->digits[i] + b->digits[i];
    }
 
  r->exponent = a->exponent;

  dbq_num_exploded("%s Before normaliser:", r);

  // Normalise
  num_propagate_carry(r);

  dbq_num_exploded("%s After carry prop r:", r);

    
  // We will always have an overflow in the first digit as we are adding a ten's
  // complement number
  if( r->digits[0] >= 10 )
    {
      // No overflow, sign stays same
      r->digits[0] -= 10;
      r->sign = a->sign;
    }
  else
    {
      // Overflow, the sign changes
      r->sign = NUM_INVERT_SIGN(a->sign);
      num_mantissa_tens_compl(r);
    }
  
  dbq_num_exploded("%s After adjust r:", r);
  
  // Normalise
  num_normalise(r);

  dbq_num_exploded("%s After normalise r:", r);
  dbq_num("%s Exit r:", r);

}

//------------------------------------------------------------------------------
//
// Add two floats, the sign is handled here.
//

void num_add(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  // If the signs are the same then we add the mantissas:
  //
  // a+b
  // -a + -b
  //
  // If we have different signs:
  //
  // -a+b
  // a + -b
  //
  // These are subtractions
  //
  dbq("");
  
  if( (a->sign) == (b->sign) )
    {
      num_add_pos(a, b, r);
      r->sign = a->sign;
    }
  else
    {
      // Different signs, so arrange the sum to be a difference
      
      if( NUM_IS_NEGATIVE(a) )
	{
	  // a negative, b positive
	  // swap to make this an x-y type subtraction
	  num_sub_pos(b, a, r);
	}
      else
	{
	  num_sub_pos(b, a, r);
	}
    }
}

//------------------------------------------------------------------------------
//
//
//   x -  y   => x - y
//   x - -y   => x + y
//  -x -  y   => -(x+y)
//  -x - -y   => y - x
//

#define SIG_P_P 0
#define SIG_P_N 1
#define SIG_N_P 2
#define SIG_N_N 3


void num_sub(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r)
{
  int signsig = 0;

  dbq_num_exploded("%s a:", a);
  dbq_num_exploded("%s b:", b);

  if( NUM_IS_NEGATIVE(a) )
    {
      dbq("a -ve");
      signsig |= 2;
    }
  else
    {
      dbq("a +ve (%d %02X", a->sign, a->sign);
    }

  if( NUM_IS_NEGATIVE(b) )
    {
      dbq("b -ve");
      signsig |= 1;
    }
  else
    {
      dbq("b +ve");
    }

  dbq("Sign sig:%d", signsig);

  switch(signsig)
    {
    case SIG_P_P:
      num_sub_pos(a, b, r);
      break;

    case SIG_P_N:
      num_add_pos(a, b, r);
      r->sign = NUM_SIGN_POSITIVE;
      break;

    case SIG_N_P:
      // a negative, b positive
      // swap to make this an x-y type subtraction
      num_add_pos(a, b, r);
      r->sign = NUM_SIGN_NEGATIVE;
      break;

    case SIG_N_N:
      // b is positive so force it
      b->sign = NUM_SIGN_POSITIVE;
      num_sub_pos(b, a, r);
      break;
    }

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

void num_sqr(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = sqrtl(d);
  num_from_double(r, d);
}

void num_sin(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = sinl(d);
  num_from_double(r, d);
}

void num_cos(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = cosl(d);
  num_from_double(r, d);
}

void num_tan(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = tanl(d);
  num_from_double(r, d);
}

void num_asin(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = asinl(d);
  num_from_double(r, d);
}

void num_acos(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = acosl(d);
  num_from_double(r, d);
}

void num_atan(NOPL_FLOAT *a, NOPL_FLOAT *r)
{
  long double d;

  d = num_to_double(a);
  d = atanl(d);
  num_from_double(r, d);
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

long double num_to_double(NOPL_FLOAT *a)
{
  long double res = 0.0;

  for(int i=0; i< NUM_MAX_DIGITS; i++)
    {
      res *= 10.0L;
      res += (long double)(a->digits[i]);
      dbq("Mantissa:%Lf digit:%d", res, a->digits[i]);
    }

  dbq("Mantissa:%Lf", res);
  res /= NUM_MAX_DIGITS_POWER10;
  res *= 10.0L;

  dbq("Adjusted%Lf", res);
    
  if( a->sign )
    {
      res = -res;
    }

  dbq("Signed:%Lf", res);
  res *= powl(10.0, a->exponent);

  dbq("Exp added:%Lf", res);
  
  dbq_num("in:", a);
  dbq("Out:%Lf", res);
  return(res);
}

void num_from_double(NOPL_FLOAT *a, long double d)
{
  char dstr[40];
  int m0;
  char m[40];
  int exp;
  int nscanf = 0;
  
  sprintf(dstr, "%LE", d);
  dbq("dstr:'%s'", dstr);

  if( dstr[0] == '-' )
    {
      dbq("Negative");
      if( (nscanf = sscanf(dstr, "-%d.%[^E]E%d", &m0, m, &exp))==3 )
	{
	  dbq("scanf: - %d . %s E %d", m0, m, exp);
	}
      else
	{
	  dbq("sscanf didn't work");
	  dbq("scanf n(%d): - %d . %s E %d", nscanf, m0, m, exp);
	  
	  return;
	}
    }
  else
    {
      dbq("Positive");
      if( (nscanf = sscanf(dstr, "%d.%[^E]E%d", &m0, m, &exp))==3 )
	{
	  dbq("scanf: %d . %s E %d", m0, m, exp);
	}
      else
	{
	  dbq("sscanf didn't work");
	  dbq("scanf n(%d): %d . %s E %d", nscanf, m0, m, exp);
	  
	  return;
	}
    }
  
  dbq("dstr:%s", dstr);
  
  if( d < 0.0 )
    {
      a->sign = NUM_SIGN_NEGATIVE;
    }
  else
    {
      a->sign = 0;
    }

  a->exponent = exp;


  for(int i=0; i<NUM_MAX_DIGITS; i++)
    {
      a->digits[i+1] = 0;
    }

  a->digits[0] = m0;

  for(int i=0; i<strlen(m); i++)
    {
      a->digits[i+1] = m[i]-'0';
    }
  
  dbq_num("res:", a);
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
