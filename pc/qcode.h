void qcode_get_string_push_stack(NOBJ_MACHINE *m);
int execute_qcode(NOBJ_MACHINE *m);
uint8_t qcode_next_8(NOBJ_MACHINE *m);
uint16_t qcode_next_16(NOBJ_MACHINE *m);
void push_machine_string(NOBJ_MACHINE *m, int len, char *str);

