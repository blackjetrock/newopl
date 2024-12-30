////////////////////////////////////////////////////////////////////////////////

#define NUM_MAX_DIGITS   12
#define NUM_BYTE_LENGTH  8

typedef struct _NOPL_FLOAT
{
  int8_t exponent;
  int8_t sign;
  int8_t digits[NUM_MAX_DIGITS];
} NOPL_FLOAT;


void num_add(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
