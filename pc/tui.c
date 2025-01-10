////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <termio.h>
#include <unistd.h>

#include "nopl.h"

////////////////////////////////////////////////////////////////////////////////

int16_t val = 0;

WINDOW *variable_win;
WINDOW *memory_win;
WINDOW *machine_win;
WINDOW *qcode_win;

WINDOW **tui_focus = &memory_win;

WINDOW *create_win(char *title, int h, int w, int y, int x)
{
  WINDOW *win;
  
  win = newwin(h, w, y, x);

  wborder(win, 0,0,0,0,0,0,0,0);
  wprintw(win, title);
  wrefresh(win);

  delwin(win);
  win = newwin(h-2, w-2, y+1, x+1);
  
  return(win);
}

int scr_maxx, scr_maxy;

void tui_init(void)
{
  //initscr();  
  //timeout(1);
  printf("\n%s", __FUNCTION__);
  
  initscr();

  //cbreak();
  noecho();
  //nodelay(stdscr, TRUE);

  clear();

  printw("main");

  getmaxyx(stdscr, scr_maxy, scr_maxx);
  
  refresh();
  
  variable_win = create_win("Variables", scr_maxy/2-1, scr_maxx/2-1,          0, scr_maxx/2);
  memory_win   = create_win("Memory",    scr_maxy/4-1, scr_maxx/4-1,          0,          0);
  qcode_win    = create_win("QCode",     scr_maxy/4-1, scr_maxx/2-1, scr_maxy/4,          0);
  machine_win  = create_win("Machine",   scr_maxy/4-1, scr_maxx/4-1,          0, scr_maxx/4);

  scrollok(memory_win, TRUE);
  scrollok(variable_win, TRUE);

  wrefresh(variable_win);
  wrefresh(machine_win);
  wrefresh(memory_win);

  printf("\n%s", __FUNCTION__);
  

}

void tui_end(void)
{
  printf("\n%s", __FUNCTION__);
  
  delwin(variable_win);
  delwin(memory_win);
  endwin();
  printf("\n%s, (cols:%d,rows:%d)", __FUNCTION__, scr_maxx, scr_maxy);
}
#if 0
bool kbhit(void)
{
    struct termios original;
    tcgetattr(STDIN_FILENO, &original);

    struct termios term;
    memcpy(&term, &original, sizeof(term));

    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);

    int characters_buffered = 0;
    ioctl(STDIN_FILENO, FIONREAD, &characters_buffered);

    tcsetattr(STDIN_FILENO, TCSANOW, &original);

    bool pressed = (characters_buffered != 0);

    return pressed;
}
#endif
void echoOff(void)
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void echoOn(void)
{
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

////////////////////////////////////////////////////////////////////////////////
//
// Keyboard

#define NUM_KEYS 35

struct
{
  char c;
  int k;
  int p5;
} keys[NUM_KEYS] =
  {
   {'a', 2, 6},
   {'b', 3, 6},
   {'c', 4, 6},
   {'d', 7, 6},
   {'e', 5, 6},
   {'f', 6, 6},
   {'g', 2, 5},
   {'h', 3, 5},
   {'i', 4, 5},
   {'j', 7, 5},
   {'k', 5, 5},
   {'l', 6, 5},
   {'m', 2, 4},
   {'n', 3, 4},
   {'o', 4, 4},
   {'p', 7, 4},
   {'q', 5, 4},
   {'r', 6, 4},
   {'s', 2, 3},
   {'t', 3, 3},
   {'u', 4, 3},
   {'v', 7, 3},
   {'w', 5, 3},
   {'x', 6, 3},
   {'y', 4, 2},
   {'z', 7, 2},
   {' ', 5, 2},
   { 10, 6, 2},  // EXE
   {'^', 2, 2},  // SHIFT
   {'[', 3, 2},  // DEL
   {'!', 1, 2},  // Mode
   {'+', 1, 3},  // UP
   {'-', 1, 4},  // DOWN
   {'<', 1, 5},
   {'>', 1, 6},

  };

////////////////////////////////////////////////////////////////////////////////

int16_t tui_memptr = 0x3f00;
#define TUI_STACK_MEM(XX) (m->stack[XX])

uint8_t tui_stack_byte(NOBJ_MACHINE *m, uint16_t idx)
{
  if( idx < 0 )
    {
      return(0);
    }

  if( idx > NOBJ_MACHINE_STACK_SIZE )
    {
      return(0);
    }

  return(TUI_STACK_MEM(idx));
}

#define MEM_LEN 64
#define MEM_LINE_LEN 8

void display_memory(NOBJ_MACHINE *m)
{
  char ascii[MEM_LINE_LEN+5];
  char f[2];
  f[1] = '\0';
    
  tui_memptr = val;
  wclear(memory_win);

  ascii[0] = '\0';
  
  for(int i=0; i<MEM_LEN; i++)
    {
      uint8_t byte = tui_stack_byte(m, tui_memptr);
      
      if( (i% MEM_LINE_LEN) == 0 )
	{
	  wprintw(memory_win, "%s\n%04X ", ascii, tui_memptr);
	  strcpy(ascii, "  ");
	}
      
      wprintw(memory_win, "%02X ", byte);
      f[0] = isprint(byte)?byte: '.';
      strcat(ascii, f);

      tui_memptr++;
    }
  
  wprintw(memory_win, "%s", ascii);

#if 0  
  for(int i=0; i<MEM_LEN; i++)
    {
      uint8_t byte = tui_stack_byte(m, tui_memptr+i);
      if( (i% MEM_LINE_LEN) == 0 )
	{
	  wprintw(memory_win, "\n     ", tui_memptr);
	}

      wprintw(memory_win, "%c  ", isprint(byte)?byte: '.');
    }

#endif
  
  wrefresh(memory_win);
}

void tui_display_machine(NOBJ_MACHINE *m)
{
  wclear(machine_win);
  wprintw(machine_win, "\nrta_fp:%04X ", m->rta_fp);
  wprintw(machine_win, "\nrta_sp:%04X ", m->rta_sp);
  wprintw(machine_win, "\nrta_pc:%04X ", m->rta_pc);

  wrefresh(machine_win);
}

char tui_srcline[MAX_NOPL_LINE];

char *tui_get_src_line_at(int a)
{
  FILE *fp;
  int idx;
  char line[MAX_NOPL_LINE];
  
  fp = fopen("intcode.txt", "r");

  if( fp == NULL )
    {
      return(NULL);
    }

    while(!feof(fp) )
    {
      int16_t mp;

      if( fgets(line, sizeof(line), fp)== NULL )
	{
	  continue;
	}
      
      if( sscanf(line, "QCode idx:%X --++ %s --++", &idx, tui_srcline) )
	{
	  return(tui_srcline);
	}
    }
    
    fclose(fp);
    return(NULL);
}

void tui_display_qcode(NOBJ_MACHINE *m)
{
  char *src;
  int i = m->rta_pc;
  NOBJ_QCODE *qc = &(m->stack[m->rta_pc]);
  
  wclear(qcode_win);
  wprintw(qcode_win, "\n%s", qcode_name(m->stack[m->rta_pc]));
  wprintw(qcode_win, "\n%04X: %02X ", m->stack[m->rta_pc]);
  
  if( (src = tui_get_src_line_at(m->rta_pc)) != NULL )
    {
      wprintw(qcode_win, "\n\n%s", src);
    }
  
  wprintw(qcode_win, "\n%s", decode_qc_txt(&i, &qc));
  wrefresh(qcode_win);
}

NOPL_FLOAT tui_num_from_mem(uint8_t *mp)
{
  NOPL_FLOAT n;
  int b;

  n.sign = *(mp++);
  n.exponent = *(mp++);
  
  // Pop digits
  for(int i = (NUM_MAX_DIGITS/2)-1; i>=0; i--)
    {
      b = *(mp++);
      n.digits[i*2] = b >> 4;
      n.digits[i*2+1] = b & 0xF;
    }

  return(n);
}

char *tui_string_at(uint8_t *mp)
{
  return("...");
}

//------------------------------------------------------------------------------

void tui_display_variables(NOBJ_MACHINE *m)
{
  FILE *fp;
  char line[300];
  char localglobal[20];
  char class[50];
  char type[50];
  char ref[50];
  int  max_string;
  int  max_array;
  int  num_indices;
  int  offset;
  
  int i;
  char varname[50];

  wclear(variable_win);
  
  fp = fopen("vars.txt", "r");

  if( fp == NULL )
    {
      return;
    }

  while(!feof(fp) )
    {
      int16_t mp;
      
      if( fgets(line, sizeof(line), fp)== NULL )
	{
	  continue;
	}

      if( sscanf(line, "%d: VAR: ' %[^']' %s %s %s max_str: %d max_ary: %d num_ind: %d offset:%X", &i, varname, class, type, ref, &max_string, &max_array, &num_indices, &offset) == 9 )
	{
	  mp = offset+m->rta_fp;
	  if( strcmp(type, "Integer")==0 )
	    {
	      wprintw(variable_win, "\n(Addr:%04X) %s:%04X", mp, varname, (m->stack[mp])*256+(m->stack[mp+1]));
	    }
	  if( strcmp(type, "String")==0 )
	    {
	      wprintw(variable_win, "\n(Addr:%04X) %s:%s", mp, varname, tui_string_at(&(m->stack[mp])));
	    }
	  
	  if( strcmp(type, "Float")==0 )
	    {
	      NOPL_FLOAT num;

	      for(int i=0; i<8; i++)
		{
		  dbq(" %02X", m->stack[mp+i]);
		}
	      num = tui_num_from_mem(&(m->stack[mp]));

	      wprintw(variable_win, "\n(Addr:%04X) %s:%s", mp, varname, num_as_text(&num, ""));
	    }
	}
    }

  fclose(fp);

  wrefresh(variable_win);
}

//------------------------------------------------------------------------------

void tui_step(NOBJ_MACHINE *m, int *done)
{
  int donek = 0;
  
  while(!donek)
    {
      int c;
      
      display_memory(m);
      tui_display_machine(m);
      tui_display_variables(m);
      tui_display_qcode(m);
      
      c = wgetch(stdscr);
      
      // Display variables
      switch(c)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  val *= 16;
	  val += (c-'0');
	  display_memory(m);
	  break;

	case 'a':
	case 'b':
	case 'c':	
	case 'd': 
	case 'e':
	case 'f':
	  val *= 16;
	  val += (c-'a'+10);
	  display_memory(m);
	  break;
	  
	case 'q':
	  *done = 1;
	  donek = 1;
	  break;

	case '\n':
	  donek = 1;
	  break;
	  
	case 'm':
	  tui_focus = &memory_win;
	  display_memory(m);
	  break;

	case 'M':
	  tui_focus = &machine_win;
	  tui_display_machine(m);
	  break;
	  
	case 'v':
	  tui_focus = &variable_win;
	  tui_display_variables(m);
	  break;
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
