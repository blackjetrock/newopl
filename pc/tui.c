////////////////////////////////////////////////////////////////////////////////

#include <ncurses.h>
#include <termio.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

void tui_init(void)
{
  //initscr();  
  //timeout(1);
  printf("\nINIT TUI");
  initscr();

  cbreak();
  noecho();
  //nodelay(stdscr, TRUE);

  clear();

  int c = wgetch(stdscr);
    //scrollok(stdscr, TRUE);
}

void tui_end(void)
{
  endwin();
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
