int scan_expression(void);
int check_function(int *index);
int scan_function(char *cmd_dest);
int next_composite_line(FILE *fp);
int scan_procdef(void);
int scan_cline(void);

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
