void read_proc_file(FILE *fp, NOBJ_PROC *p);
void read_ob3_header(FILE *fp);
int read_item(FILE *fp, void *ptr, int n, size_t size);
int read_item_16(FILE *fp, uint16_t *ptr);

char *decode_vartype(NOBJ_VARTYPE t);
void init_machine(NOBJ_MACHINE *m);
void push_proc_on_stack(NOBJ_PROC *p, NOBJ_MACHINE *machine);
void push_machine_8(NOBJ_MACHINE *m, uint8_t v);
void push_machine_16(NOBJ_MACHINE *m, int16_t v);
uint8_t pop_machine_8(NOBJ_MACHINE *m);



uint16_t pop_discard_sp_int(NOBJ_MACHINE *m, uint16_t sp);
uint16_t pop_discard_sp_float(NOBJ_MACHINE *m, uint16_t sp);
uint16_t pop_discard_sp_str(NOBJ_MACHINE *m, uint16_t sp);

uint16_t get_machine_16(NOBJ_MACHINE *m, uint16_t sp, uint16_t *v);
uint16_t get_machine_8(NOBJ_MACHINE *m, uint16_t sp, uint8_t *v);
void error(char *fmt, ...);

int datatype_length(int type, int next_byte);

#define STACK_ENTRY_16(AAA)  ((m->stack[AAA] << 8) | (m->stack[AAA+1] & 0x0F))
