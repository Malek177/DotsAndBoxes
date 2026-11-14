/*
 * display.c - Terminal output for Dots and Boxes (see display.h).
 */
#include "display.h"

#include <stdio.h>

/* ANSI escape sequences (understood by the Linux console and xterm). */
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RED "\033[1;31m"
#define ANSI_CYAN "\033[1;36m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_CLEAR "\033[2J\033[H" /* clear screen, cursor to top-left */

/* Module-private setting (static = internal linkage, invisible elsewhere). */
static bool fancy_output = true;

void display_init(bool fancy) { fancy_output = fancy; }

/* Prints s wrapped in colour code only when fancy output is on. */
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

void display_title(void) {
  put_colored(ANSI_BOLD, "=== DOTS AND BOXES === By: Malek Sakr & Jasson Matta.");
  printf("\n\n");
}

void display_clear(void) {
  if (fancy_output) {
    fputs(ANSI_CLEAR, stdout);
    display_title(); /* keep the title at the top of every fresh screen */
  }
}

/* Draws one line segment: '-' or '|' if drawn, a space otherwise. */
static void put_line(const Board *b, Line line, const Line *last, char symbol) {
  if (!board_has_line(b, line)) {
    putchar(' ');
    return;
  }
  char s[2] = {symbol, '\0'};
  if (same_line(last, line)) {
    put_colored(ANSI_GREEN, s); /* highlight the most recent move */
  } else {
    putchar(symbol);
  }
}

void display_board(const Board *b, const Line *last) {
  int rows = board_rows(b);
  int cols = board_cols(b);

  /* Column numbers of the dots. */
  printf(" ");
  for (int c = 0; c <= cols; c++) {
    char s[3] = {' ', (char)('0' + c), '\0'};
    put_colored(ANSI_YELLOW, s);
  }
  printf("\n");

  for (int r = 0; r <= rows; r++) {
    /* Dot row: "0 .-. . ." */
    char label[3] = {(char)('0' + r), ' ', '\0'};
    put_colored(ANSI_YELLOW, label);
    for (int c = 0; c <= cols; c++) {
      putchar('.');
      if (c < cols) {
        Line h = {LINE_HORIZONTAL, r, c};
        put_line(b, h, last, '-');
      }
    }
    printf("\n");

    if (r == rows) {
      break; /* no boxes below the last row of dots */
    }

    /* Box row: "  |A| |" */
    printf("  ");
    for (int c = 0; c <= cols; c++) {
      Line v = {LINE_VERTICAL, r, c};
      put_line(b, v, last, '|');
      if (c < cols) {
        Player owner = board_box_owner(b, r, c);
        if (owner == PLAYER_NONE) {
          putchar(' ');
        } else {
          char s[2] = {display_player_char(owner), '\0'};
          put_colored(player_color(owner), s);
        }
      }
    }
    printf("\n");
  }
}

void display_scores(const Board *b) {
  printf("Player ");
  put_colored(ANSI_RED, "A");
  printf(" score: %d\n", board_score(b, PLAYER_A));
  printf("Player ");
  put_colored(ANSI_CYAN, "B");
  printf(" score: %d\n", board_score(b, PLAYER_B));
}

void display_separator(void) {
  printf("**********************************************************\n");
}

void display_prompt(Player p) {
  char s[2] = {display_player_char(p), '\0'};
  printf("Player ");
  put_colored(player_color(p), s);
  printf("'s turn. Enter the row and column of the first dot "
         "and the second dot (e.g. 0 0 0 1): ");
  fflush(stdout); /* prompt has no '\n', so flush it before reading */
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
      msg = "Invalid move: dots must be next to each other "
            "(horizontally or vertically, not diagonally).";
      break;
    case MOVE_ALREADY_DRAWN:
      msg = "Invalid move: that line is already drawn.";
      break;
    case MOVE_OK:
      return;
  }
  put_colored(ANSI_RED, msg);
  printf(" Try again.\n");
}

void display_bad_format(void) {
  put_colored(ANSI_RED, "Please enter exactly 4 numbers, e.g. 0 0 0 1.");
  printf("\n");
}

void display_bonus_turn(Player p, int boxes) {
  printf("Player %c completed %d box%s and plays again!\n",
         display_player_char(p), boxes, boxes == 1 ? "" : "es");
}

void display_winner(const Board *b) {
  int a = board_score(b, PLAYER_A);
  int bs = board_score(b, PLAYER_B);
  display_separator();
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
