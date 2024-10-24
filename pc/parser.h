#define dbprintf(fmt...) dbpf(__FUNCTION__, fmt)
void dbpf(const char *caller, char *fmt, ...);

int scan_addr_name(void);
int check_addr_name(int *index);

int scan_expression(int *num_commas, int ignore_comma);
int check_function(int *index);
int scan_function(char *cmd_dest);
int next_composite_line(FILE *fp);
int scan_procdef(void);
int scan_cline(void);
int scan_integer(int *intdest);
int check_expression(int *index, int ignore_comma);
int check_declare(int *index);
int scan_declare(void);
void syntax_error(char *fmt, ...);
NOBJ_VARTYPE function_arg_type_n(char *fname, int n);
int function_num_args(char *fname);
NOBJ_VARTYPE function_return_type(char *fname);
int token_is_function(char *token, char **tokstr);
int token_is_float(char *token);
int token_is_integer(char *token);
int token_is_variable(char *token);
int token_is_string(char *token);

void finalise_expression(void);
void output_expression_start(char *expr);
void process_token(OP_STACK_ENTRY *token);
void parser_check(void);
void indent_more(void);


NOBJ_VARTYPE char_to_type(char ch);
char type_to_char(NOBJ_VARTYPE t);
extern NOBJ_VARTYPE expression_type;

#define MAX_EXP_TYPE_STACK  20

extern char cline[MAX_NOPL_LINE];
extern int cline_i;
extern FILE *ofp;
extern FILE *chkfp;
extern NOBJ_VARTYPE exp_type_stack[MAX_EXP_TYPE_STACK];
extern int exp_type_stack_ptr;
extern char current_expression[200];

#define MAX_EXP_BUFFER   2000
enum
  {
    EXP_BUFF_ID_TKN = 1,
    EXP_BUFF_ID_NONE,
    EXP_BUFF_ID_SUB_START,
    EXP_BUFF_ID_SUB_END,
    EXP_BUFF_ID_VARIABLE,
    EXP_BUFF_ID_INTEGER,
    EXP_BUFF_ID_FLT,
    EXP_BUFF_ID_STR,
    EXP_BUFF_ID_FUNCTION,
    EXP_BUFF_ID_OPERATOR,
    EXP_BUFF_ID_AUTOCON,
    EXP_BUFF_ID_COMMAND,
    EXP_BUFF_ID_PROC_CALL,
    EXP_BUFF_ID_ASSIGN,
    EXP_BUFF_ID_CONDITIONAL,
    EXP_BUFF_ID_RETURN,
    EXP_BUFF_ID_VAR_ADDR_NAME,
    EXP_BUFF_ID_MAX,
  };

extern char *exp_buffer_id_str[];

extern int node_id_index;
extern EXP_BUFFER_ENTRY exp_buffer[MAX_EXP_BUFFER];
extern int exp_buffer_i;
extern EXP_BUFFER_ENTRY exp_buffer2[MAX_EXP_BUFFER];
extern int exp_buffer2_i;

#define MAX_OPERATOR_TYPES 3
#define IMMUTABLE_TYPE     1
#define   MUTABLE_TYPE     0

typedef struct _OP_INFO
{
  char          *name;
  int           precedence;
  int           left_assoc;
  int           immutable;                // Is the operator type mutable?
  int           assignment;               // Special code for assignment
  NOBJ_VARTYPE  type[MAX_OPERATOR_TYPES];
  int           qcode;                     // Easily translatable qcodes
} OP_INFO;

extern OP_INFO  op_info[];
extern int num_operators(void);

//#define NUM_OPERATORS (sizeof(op_info)/sizeof(struct _OP_INFO))
#define NUM_OPERATORS num_operators()

void init_op_stack_entry(OP_STACK_ENTRY *op);

#define IGNORE_COMMA  1
#define HEED_COMMA    0
