/*
 * board.h - Board state and game rules (no input/output).
 * A board of R x C boxes has (R+1) x (C+1) dots. A line is stored by its
 * direction and its top/left dot.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include <stdbool.h>

#define DEFAULT_ROWS 4
#define DEFAULT_COLS 5
#define MIN_SIZE 1
#define MAX_SIZE 9

typedef enum { PLAYER_NONE = 0, PLAYER_A = 1, PLAYER_B = 2 } Player;

typedef enum { LINE_HORIZONTAL, LINE_VERTICAL } Orientation;

typedef struct {
  Orientation orient;
  int r;
  int c;
} Line;

typedef enum {
  MOVE_OK,
  MOVE_OUT_OF_RANGE,
  MOVE_SAME_DOT,
  MOVE_NOT_ADJACENT,
  MOVE_ALREADY_DRAWN
} MoveStatus;

typedef struct Board Board;

Board *board_create(int rows, int cols);
void board_destroy(Board *b);

/* A separate copy the bot can try moves on. */
Board *board_copy(const Board *b);

int board_rows(const Board *b);
int board_cols(const Board *b);

/* Checks a move between two dots and turns it into a Line. */
MoveStatus board_validate_move(const Board *b, int r1, int c1, int r2,
                               int c2, Line *out);

bool board_has_line(const Board *b, Line line);

/* Draws a line and returns how many boxes it completed (0, 1 or 2). */
int board_draw_line(Board *b, Line line, Player p);

/* Takes back the most recently drawn line and any boxes it completed. */
void board_undo_line(Board *b, Line line);

int board_box_sides(const Board *b, int r, int c);
Player board_box_owner(const Board *b, int r, int c);
int board_score(const Board *b, Player p);
bool board_is_full(const Board *b);
int board_lines_left(const Board *b);

#endif
