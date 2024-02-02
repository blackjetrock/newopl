// New OPL object code definitions.
//
// This is backwards compatible with QCode where possible.
//

typedef uint8_t NOBJ_PARAMETER_TYPE;

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

typedef uint8_t NOBJ_VARTYPE;

enum
  {
   NOBJ_VARTYPE_INT    = 0x00,
   NOBJ_VARTYPE_FLT    = 0x01,
   NOBJ_VARTYPE_STR    = 0x02,
   NOBJ_VARTYPE_INTARY = 0x03,
   NOBJ_VARTYPE_FLTARY = 0x04,
   NOBJ_VARTYPE_STRARY = 0x05,
  };

typedef uint16_t NOBJ_VARADDR;

typedef struct _NOBJ_VARNAME_ENTRY
{
  uint8_t len;
  uint8_t varname[NOBJ_VARNAME_MAXLEN];
  NOBJ_VARTYPE  type;
  NOBJ_VARADDR  address;
} NOBJ_VARNAME_ENTRY;
