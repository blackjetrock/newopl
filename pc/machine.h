void push_machine_string(NOBJ_MACHINE *m, int len, char *str);
void push_machine_8(NOBJ_MACHINE *m, uint8_t v);
void push_machine_16(NOBJ_MACHINE *m, int16_t v);
void push_machine_string(NOBJ_MACHINE *m, int len, char *str);
uint8_t pop_machine_8(NOBJ_MACHINE *m);
uint16_t pop_machine_16(NOBJ_MACHINE *m);
void pop_machine_string(NOBJ_MACHINE *m, uint8_t *len, char *str);
NOBJ_INT pop_machine_int(NOBJ_MACHINE *m);

uint16_t stack_entry_16(NOBJ_MACHINE *m, uint16_t ptr);
uint8_t stack_entry_8(NOBJ_MACHINE *m, uint16_t ptr);
uint16_t get_machine_16(NOBJ_MACHINE *m, uint16_t sp, uint16_t *v);
uint16_t get_machine_8(NOBJ_MACHINE *m, uint16_t sp, uint8_t *v);
uint16_t pop_sp_8(NOBJ_MACHINE *m, uint16_t sp, uint8_t *val);
uint8_t pop_machine_8(NOBJ_MACHINE *m);
uint16_t pop_machine_16(NOBJ_MACHINE *m);
uint16_t pop_discard_sp_int(NOBJ_MACHINE *m, uint16_t sp);
uint16_t pop_discard_sp_float(NOBJ_MACHINE *m, uint16_t sp);
uint16_t pop_discard_sp_str(NOBJ_MACHINE *m, uint16_t sp);
void push_machine_num(NOBJ_MACHINE *m, NOPL_FLOAT *n);
NOPL_FLOAT pop_machine_num(NOBJ_MACHINE *m);

uint16_t pop_discard_sp_int(NOBJ_MACHINE *m, uint16_t sp);
uint16_t pop_discard_sp_float(NOBJ_MACHINE *m, uint16_t sp);
uint16_t pop_discard_sp_str(NOBJ_MACHINE *m, uint16_t sp);

uint16_t get_machine_16(NOBJ_MACHINE *m, uint16_t sp, uint16_t *v);
uint16_t get_machine_8(NOBJ_MACHINE *m, uint16_t sp, uint8_t *v);
