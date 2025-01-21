////////////////////////////////////////////////////////////////////////////////
//
//

#define NOPL_LOGICAL_FILE_BUFFER_LEN  256
#define NOPL_NUM_LOGICAL_FILES          4
#define NOPL_MAX_FILE_NAME             32
#define NOPL_MAX_FIELD_NAME            NOBJ_VARNAME_MAXLEN
#define NOPL_FIELD_DELIMITER            9

//------------------------------------------------------------------------------

// Logical file
//

typedef struct _NOPL_LOGICAL_FILE
{
  char     name[NOPL_MAX_FILE_NAME];
  int      open;                                  // Is file open?
  int      buffer_size;                           // How many bytes in buffer?
  uint8_t  buffer[NOPL_LOGICAL_FILE_BUFFER_LEN];

  // Each file has a list of field names and a pool of text
  char         *field_name[NOPL_MAX_FIELDS];
  int          num_field_names;
  NOBJ_VARTYPE field_type[NOPL_MAX_FIELDS];
  
  int          fname_pool_i;
  char         field_name_charpool[NOPL_MAX_FIELDS*(NOPL_MAX_FIELD_NAME+1)];
} NOPL_LOGICAL_FILE;


// Logical file info
extern NOPL_LOGICAL_FILE logical_file_info[NOPL_NUM_LOGICAL_FILES];
void init_logical_files(void);
void logfile_store_field_names(NOBJ_MACHINE *m, int logfile, uint8_t *flist);
int logfile_get_field_index(NOBJ_MACHINE *m, int logfile, char *field_name);
void logfile_put_field_as_str(NOBJ_MACHINE *m, int logfile, char *field_name, char *field_val);
char *logfile_get_field_as_str(NOBJ_MACHINE *m, int logfile, char *field_name);
