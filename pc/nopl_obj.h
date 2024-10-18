// New OPL object code definitions.
//
// This is backwards compatible with QCode where possible.
//



#define NOPL_MAX_TOKEN           80
#define NOPL_MAX_OP_STACK        50
#define NOPL_MAX_LOCAL           16
#define NOPL_MAX_GLOBAL          16


typedef uint8_t NOBJ_PARAMETER_TYPE;
typedef uint16_t NOBJ_INT;

typedef struct _NOBJ_VAR_SPACE_SIZE
{
  uint16_t size;
} NOBJ_VAR_SPACE_SIZE;

typedef struct _NOBJ_QCODE_SPACE_SIZE
{
  uint16_t size;
} NOBJ_QCODE_SPACE_SIZE;

typedef struct _NOBJ_NUM_PARAMETERS
{
  uint8_t num;
} NOBJ_NUM_PARAMETERS;

typedef struct _NOBJ_GLOBAL_VARNAME_SIZE
{
  uint16_t size;
} NOBJ_GLOBAL_VARNAME_SIZE;

typedef struct _NOBJ_EXTERNAL_VARNAME_SIZE
{
  uint16_t size;
} NOBJ_EXTERNAL_VARNAME_SIZE;

typedef struct _NOBJ_STRLEN_FIXUP_SIZE
{
  uint16_t size;
} NOBJ_STRLEN_FIXUP_SIZE;

typedef struct _NOBJ_ARYSZ_FIXUP_SIZE
{
  uint16_t size;
} NOBJ_ARYSZ_FIXUP_SIZE;

typedef uint8_t NOBJ_VARTYPE;

enum
  {
   NOBJ_VARTYPE_INT     = 0x00,
   NOBJ_VARTYPE_FLT     = 0x01,
   NOBJ_VARTYPE_STR     = 0x02,
   NOBJ_VARTYPE_INTARY  = 0x03,
   NOBJ_VARTYPE_FLTARY  = 0x04,
   NOBJ_VARTYPE_STRARY  = 0x05,
   NOBJ_VARTYPE_UNKNOWN = 0x10,
   NOBJ_VARTYPE_VOID    = 0x11,
  };

typedef uint16_t NOBJ_ADDR;

typedef struct _NOBJ_VARNAME_ENTRY
{
  uint8_t len;
  uint8_t varname[NOBJ_VARNAME_MAXLEN];
  NOBJ_VARTYPE  type;
  NOBJ_ADDR  address;
} NOBJ_VARNAME_ENTRY;

typedef uint8_t NOBJ_STRLEN_FIXUP_LEN;

typedef struct _NOBJ_STRLEN_FIXUP_ENTRY
{
  NOBJ_ADDR           address;
  NOBJ_STRLEN_FIXUP_LEN  len;
} NOBJ_STRLEN_FIXUP_ENTRY;

typedef uint16_t NOBJ_ARYSZ_FIXUP_LEN;

typedef struct _NOBJ_ARYSZ_FIXUP_ENTRY
{
  NOBJ_ADDR              address;
  NOBJ_ARYSZ_FIXUP_LEN   len;
} NOBJ_ARYSZ_FIXUP_ENTRY;

typedef uint8_t NOBJ_QCODE;

typedef struct _NOBJ_PROC
{
  NOBJ_VAR_SPACE_SIZE        var_space_size;
  NOBJ_QCODE_SPACE_SIZE      qcode_space_size;
  NOBJ_NUM_PARAMETERS        num_parameters;
  NOBJ_PARAMETER_TYPE        parameter_types[NOBJ_MAX_PARAMETERS];
  NOBJ_GLOBAL_VARNAME_SIZE   global_varname_size;
  int                        global_varname_num;
  NOBJ_VARNAME_ENTRY         global_varname[NOBJ_GLOBAL_VARNAME_SPACE_MAX];
  NOBJ_EXTERNAL_VARNAME_SIZE external_varname_size;
  int                        external_varname_num;
  NOBJ_VARNAME_ENTRY         external_varname[NOBJ_EXTERNAL_VARNAME_SPACE_MAX];
  NOBJ_STRLEN_FIXUP_SIZE     strlen_fixup_size;
  int                        strlen_fixup_num;
  NOBJ_STRLEN_FIXUP_ENTRY    strlen_fixup[NOBJ_STRLEN_FIXUP_MAX];
  NOBJ_ARYSZ_FIXUP_SIZE      arysz_fixup_size;
  int                        arysz_fixup_num;
  NOBJ_STRLEN_FIXUP_ENTRY    arysz_fixup[NOBJ_ARYSZ_FIXUP_MAX];
  NOBJ_QCODE                 *qcode;  
} NOBJ_PROC;

typedef uint16_t NOBJ_SP;

// An execution machine. This is what the QCode executes in
     
typedef struct _NOBJ_MACHINE
{
  // The machine stack where PROCs are loaded and executed
  uint8_t stack[NOBJ_MACHINE_STACK_SIZE];
  
  // Stack pointer (where next byte will be loaded

    NOBJ_SP rta_sp;
    int rta_pc;
    int rta_fp;
} NOBJ_MACHINE;


////////////////////////////////////////////////////////////////////////////////
// Shunting algorithm operator stack
// Name is the token string
// type is the type of the expression that is being parsed, nbot this
// entry specifically. It is determined as each expression is processed
// For instance, it starts unknown and could then be INT, then move to FLT
// if a decimal point is seen.
// It is needed for tokens like '=' to be selected correctly, as there are
// different tokens for string = and int =
//
//
// Information about a variable
//
////////////////////////////////////////////////////////////////////////////////

typedef struct _NOBJ_VAR_INFO
{
  char name[NOBJ_VARNAME_MAXLEN];
  int is_global;
  int is_ref;
  int is_array;
  int is_integer;
  int is_float;
  int is_string;
  int max_array;
  int max_string;
  int num_indices;
  NOBJ_VARTYPE type;
  uint16_t offset;    // Offset from FP
} NOBJ_VAR_INFO;

typedef struct _OP_STACK_ENTRY
{
  char           name[30];
  int            buf_id;
  NOBJ_INT       integer;
  NOBJ_VARTYPE   type;       // Original type
  NOBJ_VARTYPE   req_type;   // Required type
  struct _NOBJ_VAR_INFO  vi;
} OP_STACK_ENTRY;

#define MAX_EXP_BUF_P   12


typedef struct _EXP_BUFFER_ENTRY
{
  char name[40];
  OP_STACK_ENTRY op;
  int node_id;
  int p_idx;
  int p[MAX_EXP_BUF_P];
  int nxt;
} EXP_BUFFER_ENTRY;

