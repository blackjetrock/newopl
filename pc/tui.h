////////////////////////////////////////////////////////////////////////////////
//

#define NORMAL_LAYOUT 0
#define DEBUG_LAYOUT  1

void tui_end(void);
void tui_init(void);
void tui_step(NOBJ_MACHINE *m, int *done);

extern WINDOW *variable_win;
extern WINDOW *memory_win;
extern WINDOW *machine_win;
extern WINDOW *qcode_win;
extern WINDOW *output_win;
extern WINDOW *printer_win;
