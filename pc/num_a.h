////////////////////////////////////////////////////////////////////////////////

#define NUM_MAX_DIGITS   12
#define NUM_BYTE_LENGTH  8

typedef struct _NOPL_FLOAT
{
  int8_t exponent;
  int8_t sign;
  int8_t digits[NUM_MAX_DIGITS];
} NOPL_FLOAT;

#define NUM_POSITIVE(NN) ((NN->sign) == 0)
#define NUM_NEGATIVE(NN) ((NN->sign) != 0)

void num_add(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void num_sub(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void dbq_num(char *text, NOPL_FLOAT *n);
void dbq_num_exploded(char *text, NOPL_FLOAT *n);
int num_eq(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_gt(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_ne(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_gt(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_lt(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_gte(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_lte(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_or(NOPL_FLOAT *a, NOPL_FLOAT *b);
int num_not(NOPL_FLOAT *a);
int num_and(NOPL_FLOAT *a, NOPL_FLOAT *b);





