////////////////////////////////////////////////////////////////////////////////

#define NUM_MAX_DIGITS   12
#define NUM_MAX_DIGITS_POWER10 1e12
#define NUM_BYTE_LENGTH  8
#define NUM_AS_TEXT_LEN       (NUM_MAX_DIGITS+1+1+2+1+1)

typedef struct _NOPL_FLOAT
{
  int8_t exponent;
  int8_t sign;
  int8_t digits[NUM_MAX_DIGITS];
} NOPL_FLOAT;

#define NUM_SIGN_NEGATIVE 0x80
#define NUM_SIGN_POSITIVE 0x00

#define NUM_INVERT_SIGN(SS) (NUM_SIGN_NEGATIVE - SS)
#define NUM_IS_NEGATIVE(NN) (((uint8_t)(NN->sign))==NUM_SIGN_NEGATIVE)
#define NUM_IS_POSITIVE(NN) (((uint8_t)(NN->sign))==NUM_SIGN_POSITIVE)
#define NUM_POSITIVE(NN) NUM_IS_POSITIVE(NN)
#define NUM_NEGATIVE(NN) NUM_IS_NEGATIVE(NN)

void num_add(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void num_sub(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void num_mul(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void num_div(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void num_pow(NOPL_FLOAT *a, NOPL_FLOAT *b, NOPL_FLOAT *r);
void num_pi(NOPL_FLOAT *r);

void dbq_num_f(const char *caller, char *text, NOPL_FLOAT *n);
void dbq_num_exploded_f(const char *caller, char *text, NOPL_FLOAT *n);
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
long double num_to_double(NOPL_FLOAT *a);
void num_from_double(NOPL_FLOAT *a, long double d);
void num_log(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_log10(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_sin(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_cos(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_tan(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_abs(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_asin(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_acos(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_atan(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_sin(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_sqr(NOPL_FLOAT *a, NOPL_FLOAT *r);
void num_mantissa_tens_compl(NOPL_FLOAT *n);
void num_mantissa_tens_compl_digits(int n, int8_t *digits);
void num_propagate_carry_digits(int8_t *digits, int num_digits);
void num_propagate_carry(NOPL_FLOAT *n, int num_digits);
void num_db_digits(char *text, int n, int8_t *d);
void num_num_to_int(int n, NOPL_FLOAT *num,  NOBJ_INT *i);
char *num_as_text(NOPL_FLOAT *n, char *text);
void num_int_to_num(int n, NOBJ_INT *i, NOPL_FLOAT *num);
int num_digits_zero(int n, int8_t *digits);
NOPL_FLOAT num_from_mem(uint8_t *mp);
char *num_to_text(NOPL_FLOAT *n);
void num_clear_digits(int n, int8_t *d);
NOPL_FLOAT num_from_text(char *p);
void num_normalise(NOPL_FLOAT *n);
