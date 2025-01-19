////////////////////////////////////////////////////////////////////////////////
//
//

#define NOPL_LOGICAL_FILE_BUFFER_LEN  256
#define NOPL_NUM_LOGICAL_FILES          4
#define NOPL_MAX_FILE_NAME             32

//------------------------------------------------------------------------------

// Logical file
//

typedef struct _NOPL_LOGICAL_FILE
{
  char     name[NOPL_MAX_FILE_NAME];
  int      open;                                  // Is file open?
  int      buffer_size;                           // How many bytes in buffer?
  uint8_t  buffer[NOPL_LOGICAL_FILE_BUFFER_LEN];
  uint8_t  *field_list;                           // Pointer to field list
} NOPL_LOGICAL_FILE;


// Logical file info
extern NOPL_LOGICAL_FILE logical_file_info[NOPL_NUM_LOGICAL_FILES];
void init_logical_files(void);
