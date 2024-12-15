 #define dbprintf(fmt...) dbpf(__FUNCTION__, fmt)
void dbpf(const char *caller, char *fmt, ...);
extern FILE *ptfp;


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
    EXP_BUFF_ID_OPERATOR_UNARY,
    EXP_BUFF_ID_AUTOCON,
    EXP_BUFF_ID_COMMAND,
    EXP_BUFF_ID_PROC_CALL,
    EXP_BUFF_ID_ASSIGN,
    EXP_BUFF_ID_CONDITIONAL,
    EXP_BUFF_ID_RETURN,
    EXP_BUFF_ID_VAR_ADDR_NAME,
    EXP_BUFF_ID_PRINT,
    EXP_BUFF_ID_PRINT_SPACE,
    EXP_BUFF_ID_PRINT_NEWLINE,
    EXP_BUFF_ID_LPRINT,
    EXP_BUFF_ID_LPRINT_SPACE,
    EXP_BUFF_ID_LPRINT_NEWLINE,
    EXP_BUFF_ID_IF,
    EXP_BUFF_ID_ENDIF,
    EXP_BUFF_ID_ELSE,
    EXP_BUFF_ID_ELSEIF,
    EXP_BUFF_ID_DO,
    EXP_BUFF_ID_UNTIL,
    EXP_BUFF_ID_WHILE,
    EXP_BUFF_ID_ENDWH,
    EXP_BUFF_ID_GOTO,
    EXP_BUFF_ID_TRAP,
    EXP_BUFF_ID_LABEL,
    EXP_BUFF_ID_CONTINUE,
    EXP_BUFF_ID_BREAK,
    EXP_BUFF_ID_META,
    EXP_BUFF_ID_BRAENDIF,              // Only used in qcode generation
    EXP_BUFF_ID_FIELDVAR,
    EXP_BUFF_ID_LOGICALFILE,
    EXP_BUFF_ID_ONERR,
    EXP_BUFF_ID_INPUT,
    EXP_BUFF_ID_MAX,
  };

extern char *exp_buffer_id_str[];

extern int node_id_index;
extern EXP_BUFFER_ENTRY exp_buffer[MAX_EXP_BUFFER];
extern int exp_buffer_i;
extern EXP_BUFFER_ENTRY exp_buffer2[MAX_EXP_BUFFER];
extern int exp_buffer2_i;

//------------------------------------------------------------------------------


#define MAX_OPERATOR_TYPES 6
#define IMMUTABLE_TYPE     1
#define   MUTABLE_TYPE     0

typedef struct _OP_INFO
{
  char          *name;
  int           returns_result;           // Some operators do not create a result (e.g. ':=')
  int           precedence;
  NOBJ_VARTYPE  output_type;              // If not unknown then returns this type
  int           can_be_unary;             // Can be a unary operator
  char          *unaryop;                 // Unary version of operator name
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

extern int n_lines_ok;
extern int n_lines_bad;
extern int n_lines_blank;
extern int n_stack_errors;

typedef struct _LEVEL_INFO
{
  int if_level;
  int do_level;
  int while_level;
} LEVEL_INFO;

// scan_command can scan for just trappable commands
#define SCAN_ALL          0
#define SCAN_TRAPPABLE    1

//------------------------------------------------------------------------------

#define QI_INT_SIM_FP           0x00    
#define QI_NUM_SIM_FP           0x01    
#define QI_STR_SIM_FP           0x02    
#define QI_INT_ARR_FP           0x03    
#define QI_NUM_ARR_FP           0x04    
#define QI_STR_ARR_FP           0x05    
#define QI_NUM_SIM_ABS          0x06    
#define QI_INT_SIM_IND          0x07    
#define QI_NUM_SIM_IND          0x08    
#define QI_STR_SIM_IND          0x09    
#define QI_INT_ARR_IND          0x0A    
#define QI_NUM_ARR_IND          0x0B    
#define QI_STR_ARR_IND          0x0C    
#define QI_LS_INT_SIM_FP        0x0D    
#define QI_LS_NUM_SIM_FP        0x0E    
#define QI_LS_STR_SIM_FP        0x0F    
#define QI_LS_INT_ARR_FP        0x10    
#define QI_LS_NUM_ARR_FP        0x11    
#define QI_LS_STR_ARR_FP        0x12    
#define QI_LS_NUM_SIM_ABS       0x13    
#define QI_LS_INT_SIM_IND       0x14    
#define QI_LS_NUM_SIM_IND       0x15    
#define QI_LS_STR_SIM_IND       0x16    
#define QI_LS_INT_ARR_IND       0x17    
#define QI_LS_NUM_ARR_IND       0x18    
#define QI_LS_STR_ARR_IND       0x19
#define QI_INT_FLD              0x1A    
#define QI_NUM_FLD              0x1B    
#define QI_STR_FLD              0x1C    
#define QI_LS_INT_FLD           0x1D    
#define QI_LS_NUM_FLD           0x1E    
#define QI_LS_STR_FLD           0x1F    
#define QI_STK_LIT_BYTE         0x20    
#define QI_STK_LIT_WORD         0x21    
#define QI_INT_CON              0x22    
#define QI_NUM_CON              0x23    
#define QI_STR_CON              0x24    
#define QCO_SPECIAL             0x25    
#define QCO_BREAK               0x26    
#define QCO_LT_INT              0x27    
#define QCO_LTE_INT             0x28    
#define QCO_GT_INT              0x29    
#define QCO_GTE_INT             0x2A    
#define QCO_NE_INT              0x2B    
#define QCO_EQ_INT              0x2C    
#define QCO_ADD_INT             0x2D    
#define QCO_SUB_INT             0x2E    
#define QCO_MUL_INT             0x2F    
#define QCO_DIV_INT             0x30    
#define QCO_POW_INT             0x31    
#define QCO_UMIN_INT            0x32    
#define QCO_NOT_INT             0x33    
#define QCO_AND_INT             0x34    
#define QCO_OR_INT              0x35    
#define QCO_LT_NUM              0x36    
#define QCO_LTE_NUM             0x37    
#define QCO_GT_NUM              0x38    
#define QCO_GTE_NUM             0x39    
#define QCO_NE_NUM              0x3A    
#define QCO_EQ_NUM              0x3B    
#define QCO_ADD_NUM             0x3C    
#define QCO_SUB_NUM             0x3D    
#define QCO_MUL_NUM             0x3E    
#define QCO_DIV_NUM             0x3F    
#define QCO_POW_NUM             0x40    
#define QCO_UMIN_NUM            0x41    
#define QCO_NOT_NUM             0x42    
#define QCO_AND_NUM             0x43    
#define QCO_OR_NUM              0x44    
#define QCO_LT_STR              0x45    
#define QCO_LTE_STR             0x46    
#define QCO_GT_STR              0x47    
#define QCO_GTE_STR             0x48    
#define QCO_NE_STR              0x49    
#define QCO_EQ_STR              0x4A    
#define QCO_ADD_STR             0x4B    
#define QCO_AT                  0x4C    
#define QCO_BEEP                0x4D    
#define QCO_CLS                 0x4E    
#define QCO_CURSOR              0x4F    
#define QCO_ESCAPE              0x50    
#define QCO_GOTO                0x51    
#define QCO_OFF                 0x52    
#define QCO_ONERR               0x53    
#define QCO_PAUSE               0x54    
#define QCO_POKEB               0x55    
#define QCO_POKEW               0x56    
#define QCO_RAISE               0x57    
#define QCO_RANDOMIZE           0x58    
#define QCO_STOP                0x59    
#define QCO_TRAP                0x5A    
#define QCO_APPEND              0x5B    
#define QCO_CLOSE               0x5C    
#define QCO_COPY                0x5D    
#define QCO_CREATE              0x5E    
#define QCO_DELETE              0x5F    
#define QCO_ERASE               0x60    
#define QCO_FIRST               0x61    
#define QCO_LAST                0x62    
#define QCO_NEXT                0x63    
#define QCO_BACK                0x64    
#define QCO_OPEN                0x65    
#define QCO_POSITION            0x66    
#define QCO_RENAME              0x67    
#define QCO_UPDATE              0x68    
#define QCO_USE                 0x69    
#define QCO_KSTAT               0x6A    
#define QCO_EDIT                0x6B    
#define QCO_INPUT_INT           0x6C    
#define QCO_INPUT_NUM           0x6D    
#define QCO_INPUT_STR           0x6E    
#define QCO_PRINT_INT           0x6F    
#define QCO_PRINT_NUM           0x70    
#define QCO_PRINT_STR           0x71    
#define QCO_PRINT_SP            0x72    
#define QCO_PRINT_CR            0x73    
#define QCO_LPRINT_INT          0x74    
#define QCO_LPRINT_NUM          0x75    
#define QCO_LPRINT_STR          0x76    
#define QCO_LPRINT_SP           0x77    
#define QCO_LPRINT_CR           0x78    
#define QCO_RETURN              0x79    
#define QCO_RETURN_NOUGHT       0x7A    
#define QCO_RETURN_ZERO         0x7B    
#define QCO_RETURN_NULL         0x7C    
#define QCO_PROC                0x7D    
#define QCO_BRA_FALSE           0x7E    
#define QCO_ASS_INT             0x7F    
#define QCO_ASS_NUM             0x80    
#define QCO_ASS_STR             0x81    
#define QCO_DROP_BYTE           0x82    
#define QCO_DROP_WORD           0x83    
#define QCO_DROP_NUM            0x84    
#define QCO_DROP_STR            0x85    
#define QCO_INT_TO_NUM          0x86    
#define QCO_NUM_TO_INT          0x87    
#define QCO_END_FIELDS          0x88    
#define QCO_RUN_ASSEM           0x89    
#define RTF_ADDR                0x8A    
#define RTF_ASC                 0x8B    
#define RTF_DAY                 0x8C    
#define RTF_DISP                0x8D    
#define RTF_ERR                 0x8E    
#define RTF_FIND                0x8F    
#define RTF_FREE                0x90    
#define RTF_GET                 0x91    
#define RTF_HOUR                0x92    
#define RTF_IABS                0x93    
#define RTF_INT                 0x94    
#define RTF_KEY                 0x95    
#define RTF_LEN                 0x96    
#define RTF_LOC                 0x97    
#define RTF_MENU                0x98    
#define RTF_MINUTE              0x99    
#define RTF_MONTH               0x9A    
#define RTF_PEEKB               0x9B    
#define RTF_PEEKW               0x9C    
#define RTF_RECSIZE             0x9D    
#define RTF_SECOND              0x9E    
#define RTF_IUSR                0x9F    
#define RTF_VIEW                0xA0    
#define RTF_YEAR                0xA1    
#define RTF_COUNT               0xA2    
#define RTF_EOF                 0xA3    
#define RTF_EXIST               0xA4    
#define RTF_POS                 0xA5    
#define RTF_ABS                 0xA6    
#define RTF_ATAN                0xA7    
#define RTF_COS                 0xA8    
#define RTF_DEG                 0xA9    
#define RTF_EXP                 0xAA    
#define RTF_FLT                 0xAB    
#define RTF_INTF                0xAC    
#define RTF_LN                  0xAD    
#define RTF_LOG                 0xAE    
#define RTF_PI                  0xAF    
#define RTF_RAD                 0xB0    
#define RTF_RND                 0xB1    
#define RTF_SIN                 0xB2    
#define RTF_SQR                 0xB3    
#define RTF_TAN                 0xB4    
#define RTF_VAL                 0xB5    
#define RTF_SPACE               0xB6    
#define RTF_DIR                 0xB7    
#define RTF_CHR                 0xB8    
#define RTF_DATIM               0xB9    
#define RTF_SERR                0xBA    
#define RTF_FIX                 0xBB    
#define RTF_GEN                 0xBC    
#define RTF_SGET                0xBD    
#define RTF_HEX                 0xBE    
#define RTF_SKEY                0xBF    
#define RTF_LEFT                0xC0    
#define RTF_LOWER               0xC1    
#define RTF_MID                 0xC2    
#define RTF_NUM                 0xC3    
#define RTF_RIGHT               0xC4    
#define RTF_REPT                0xC5    
#define RTF_SCI                 0xC6    
#define RTF_UPPER               0xC7    
#define RTF_SUSR                0xC8    
#define RTF_SADDR               0xC9    

// LZ QCode
#define RTF_DOW                 0xD7
#define RTF_LTPERCENT           0xCC
#define RTF_GTPERCENT           0xCD
#define RTF_PLUSPERCENT         0xCE
#define RTF_MINUSPERCENT        0xCF
#define RTF_TIMESPERCENT        0xD0
#define RTF_DIVIDEPERCENT       0xD1
#define RTF_OFFX                0xD2
#define RTF_COPYW               0xD3
#define RTF_DELETEW             0xD4
#define RTF_UDG                 0xD5
#define RTF_CLOCK               0xD6
#define RTF_DOW                 0xD7
#define RTF_FINDW               0xD8
#define RTF_MENUN               0xD9
#define RTF_WEEK                0xDA
#define RTF_ACOS                0xDB
#define RTF_ASIN                0xDC
#define RTF_DAYS                0xDD
#define RTF_MAX                 0xDE
#define RTF_MEAN                0xDF
#define RTF_MIN                 0xE0
#define RTF_STD                 0xE1
#define RTF_SUM                 0xE2
#define RTF_VAR                 0xE3
#define RTF_DAYNAME             0xE4
#define RTF_DIRW$               0xE5
#define RTF_MONTHSTR            0xE6

//------------------------------------------------------------------------------



  
extern NOBJ_VAR_INFO var_info[MAX_VAR_INFO];
extern int num_var_info;

////////////////////////////////////////////////////////////////////////////////

int scan_addr_name(void);
int check_addr_name(int *index);
int scan_literal(char *lit);
int check_literal(int *index, char *lit);
int scan_line(LEVEL_INFO levels);

void indent_none(void);
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
int check_expression_list(int *index);
int is_all_spaces(int idx);

void finalise_expression(void);
void output_expression_start(char *expr);
void process_token(OP_STACK_ENTRY *token);
void parser_check(void);
void indent_more(void);
int scan_expression_list(int *num_expressions, int insert_types);
void initialise_line_supplier(FILE *fp);
int pull_next_line(void);
void op_stack_finalise(void);
void output_if(OP_STACK_ENTRY op);
void output_generic(OP_STACK_ENTRY op, char *name, int buf_id);
int strn_match(char *s1, char *s2, int n);
int check_proc_call(int *index);
int scan_proc_call(void);
void typecheck_error(char *fmt, ...);
void fprint_var_info(FILE *fp, NOBJ_VAR_INFO *vi);
void internal_error(char *fmt, ...);
char *type_to_str(NOBJ_VARTYPE t);
void make_var_type_array(NOBJ_VARTYPE *vt);
int var_type_is_array(NOBJ_VARTYPE vt);
void dump_qcode_data(void);
void build_qcode_header(void);
int set_qcode_header_byte_at(int idx, int len, int val);
int set_qcode_header_string_at(int idx, char *str);
NOBJ_VAR_INFO *find_var_info(char *name, NOBJ_VARTYPE type);
int check_operator(int *index, int *is_comma, int ignore_comma);
int function_access_force_write(char * fname);

extern int qcode_idx;
extern int pass_number;
extern int qcode_header_len;
extern int qcode_start_idx;
extern int qcode_len;
extern int size_of_qcode_idx;
extern int procedure_has_return;
extern NOBJ_VARTYPE  procedure_type;
void dump_vars(FILE *fp);


#define FLT_INCLUDES_SIGN   0
