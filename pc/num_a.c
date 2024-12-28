////////////////////////////////////////////////////////////////////////////////
//
// Num package
//
// BCD floating point
//
////////////////////////////////////////////////////////////////////////////////

#include "num.h"

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
