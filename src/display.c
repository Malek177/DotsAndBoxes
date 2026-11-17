/*
 * display.c - Drawing the board and messages on the terminal.
 */
#define _POSIX_C_SOURCE 200809L

#include "display.h"

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RED "\033[1;31m"
#define ANSI_CYAN "\033[1;36m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_CLEAR "\033[2J\033[H"

/* Size of one box in fancy mode: 5 characters wide, 2 lines tall. */
#define FANCY_CELL_W 5
#define FANCY_CELL_H 2

#define TITLE "=== DOTS AND BOXES === By: Malek Sakr & Jasson Matta."
#define LABEL_W 2
#define FALLBACK_ROWS 24
#define FALLBACK_COLS 80

static bool fancy_output = true;
static int cell_w = 1;
static int cell_h = 1;
static int grid_rows = DEFAULT_ROWS;
static int grid_cols = DEFAULT_COLS;
static int left_pad = 0;
static int term_width = FALLBACK_COLS;

void display_init(bool fancy, int rows, int cols) {
  fancy_output = fancy;
  grid_rows = rows;
  grid_cols = cols;
  cell_w = fancy ? FANCY_CELL_W : 1;
  cell_h = fancy ? FANCY_CELL_H : 1;
}

/* Layout: how big the board is and how much space to leave around it. */
static int board_width(void) { return LABEL_W + 1 + grid_cols * (cell_w + 1); }

/* Lines used: title, column numbers, dot rows, box rows, scores, message, prompt. */
static int screen_height(void) {
  return 2
       + 1
       + (grid_rows + 1)
       + grid_rows * cell_h
       + 4
       + 1
       + 2;
}

static void terminal_size(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    *rows = ws.ws_row;
    *cols = ws.ws_col;
  } else {
    *rows = FALLBACK_ROWS;
    *cols = FALLBACK_COLS;
  }
}

static void indent(void) {
  for (int i = 0; i < left_pad; i++) {
    putchar(' ');
  }
}

static void repeat(char ch, int n) {
  for (int i = 0; i < n; i++) {
    putchar(ch);
  }
}

static void put_colored(const char *color, const char *s) {
  if (fancy_output) {
    printf("%s%s%s", color, s, ANSI_RESET);
  } else {
    fputs(s, stdout);
  }
}

static const char *player_color(Player p) {
  return (p == PLAYER_A) ? ANSI_RED : ANSI_CYAN;
}

char display_player_char(Player p) {
  return (p == PLAYER_A) ? 'A' : 'B';
}

static bool same_line(const Line *a, Line b) {
  return a != NULL && a->orient == b.orient && a->r == b.r && a->c == b.c;
}

/* Title and clearing the screen. */
void display_title(void) {
  if (fancy_output) {
    int pad = (term_width - (int)strlen(TITLE)) / 2;
    repeat(' ', pad > 0 ? pad : 0);
  }
  put_colored(ANSI_BOLD, TITLE);
  printf("\n\n");
}

void display_clear(void) {
  if (!fancy_output) {
    return;
  }
  int term_rows, term_cols;
  terminal_size(&term_rows, &term_cols);
  term_width = term_cols;
  int pad_x = (term_cols - board_width()) / 2;
  int pad_y = (term_rows - screen_height()) / 2;
  left_pad = pad_x > 0 ? pad_x : 0;

  fputs(ANSI_CLEAR, stdout);
  for (int i = 0; i < pad_y; i++) {
    putchar('\n');
  }
  display_title();
}

/* Drawing the board. */
static void put_segment(const Board *b, Line line, const Line *last,
                        char symbol, int len) {
  if (!board_has_line(b, line)) {
    repeat(' ', len);
    return;
  }
  if (fancy_output && same_line(last, line)) {
    fputs(ANSI_GREEN, stdout);
    repeat(symbol, len);
    fputs(ANSI_RESET, stdout);
  } else {
    repeat(symbol, len);
  }
}

static void put_box_inside(const Board *b, int r, int c, int row_in_box) {
  Player owner = board_box_owner(b, r, c);
  if (owner == PLAYER_NONE || row_in_box != (cell_h - 1) / 2) {
    repeat(' ', cell_w);
    return;
  }
  int before = cell_w / 2;
  repeat(' ', before);
  char s[2] = {display_player_char(owner), '\0'};
  put_colored(player_color(owner), s);
  repeat(' ', cell_w - before - 1);
}

void display_board(const Board *b, const Line *last) {
  int rows = board_rows(b);
  int cols = board_cols(b);

  indent();
  repeat(' ', LABEL_W);
  for (int c = 0; c <= cols; c++) {
    char s[2] = {(char)('0' + c), '\0'};
    put_colored(ANSI_YELLOW, s);
    if (c < cols) {
      repeat(' ', cell_w);
    }
  }
  printf("\n");

  for (int r = 0; r <= rows; r++) {
    indent();
    char label[3] = {(char)('0' + r), ' ', '\0'};
    put_colored(ANSI_YELLOW, label);
    for (int c = 0; c <= cols; c++) {
      putchar('.');
      if (c < cols) {
        Line h = {LINE_HORIZONTAL, r, c};
        put_segment(b, h, last, '-', cell_w);
      }
    }
    printf("\n");

    if (r == rows) {
      break;
    }

    for (int k = 0; k < cell_h; k++) {
      indent();
      repeat(' ', LABEL_W);
      for (int c = 0; c <= cols; c++) {
        Line v = {LINE_VERTICAL, r, c};
        put_segment(b, v, last, '|', 1);
        if (c < cols) {
          put_box_inside(b, r, c, k);
        }
      }
      printf("\n");
    }
  }
}

/* Scores, prompts and messages. */
void display_scores(const Board *b) {
  indent();
  printf("Player ");
  put_colored(ANSI_RED, "A");
  printf(" score: %d\n", board_score(b, PLAYER_A));
  indent();
  printf("Player ");
  put_colored(ANSI_CYAN, "B");
  printf(" score: %d\n", board_score(b, PLAYER_B));
}

void display_separator(void) {
  indent();
  if (fancy_output) {
    repeat('*', board_width());
    printf("\n");
  } else {
    printf("**********************************************************\n");
  }
}

void display_prompt(Player p) {
  char s[2] = {display_player_char(p), '\0'};
  indent();
  printf("Player ");
  put_colored(player_color(p), s);
  if (fancy_output) {
    printf("'s turn. Enter two dots: row col row col\n");
    indent();
    printf("(e.g. 0 0 0 1) > ");
  } else {
    printf("'s turn. Enter the row and column of the first dot "
           "and the second dot (e.g. 0 0 0 1): ");
  }
  fflush(stdout);
}

void display_move_error(MoveStatus status) {
  const char *msg = "Invalid move.";
  switch (status) {
    case MOVE_OUT_OF_RANGE:
      msg = "Invalid move: that dot is outside the grid.";
      break;
    case MOVE_SAME_DOT:
      msg = "Invalid move: you entered the same dot twice.";
      break;
    case MOVE_NOT_ADJACENT:
      msg = "Invalid move: dots must be neighbours, no diagonals.";
      break;
    case MOVE_ALREADY_DRAWN:
      msg = "Invalid move: that line is already drawn.";
      break;
    case MOVE_OK:
      return;
  }
  indent();
  put_colored(ANSI_RED, msg);
  printf("\n");
}

void display_bad_format(void) {
  indent();
  put_colored(ANSI_RED, "Please enter exactly 4 numbers, e.g. 0 0 0 1.");
  printf("\n");
}

void display_bonus_turn(Player p, int boxes) {
  indent();
  printf("Player %c completed %d box%s and plays again!\n",
         display_player_char(p), boxes, boxes == 1 ? "" : "es");
}

void display_play_again(void) {
  indent();
  printf("Play again? [Y/n]: ");
  fflush(stdout);
}

void display_winner(const Board *b) {
  int a = board_score(b, PLAYER_A);
  int bs = board_score(b, PLAYER_B);
  display_separator();
  indent();
  if (a > bs) {
    put_colored(ANSI_RED, "Player A wins!");
  } else if (bs > a) {
    put_colored(ANSI_CYAN, "Player B wins!");
  } else {
    put_colored(ANSI_BOLD, "It's a tie!");
  }
  printf(" Final score: A %d - %d B\n", a, bs);
  display_separator();
}
