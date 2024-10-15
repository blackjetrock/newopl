#define NOBJ_MAX_PROCNAME                  16
#define NOBJ_MAX_PARAMETERS               255
#define NOBJ_VARNAME_MAXLEN                 (8+1+1)
#define NOBJ_FILENAME_MAXLEN               12
#define NOBJ_GLOBAL_VARNAME_SPACE_MAX      16
#define NOBJ_EXTERNAL_VARNAME_SPACE_MAX    16
#define NOBJ_STRLEN_FIXUP_MAX              16
#define NOBJ_ARYSZ_FIXUP_MAX               16
#define NOBJ_MAX_QCODE_SIZE             65535

//#define NOBJ_MACHINE_STACK_SIZE         65535
#define NOBJ_MACHINE_STACK_SIZE         0x3eff

#define NOBJ_PRT_MAX_LINE                 400


// Informtion about a variable

typedef struct _NOBJ_VAR_INFO
{
  char name[NOBJ_VARNAME_MAXLEN];
  int is_array;
  int is_integer;
  int is_float;
  int is_string;
  int max_array;
  int max_string;
  NOBJ_VARTYPE type;
  uint16_t offset;    // Offset from FP
} NOBJ_VAR_INFO;
