/*
 * board.h - The Dots and Boxes board: pure game state and rules.
 *
 * This module does NO input/output. It only knows which lines are drawn
 * and who owns which box. Keeping it I/O-free means the same code can be
 * reused by the bot (Sprint 2) and the network version (Sprint 3), and it
 * can be unit-tested without a terminal.
 *
 * Coordinates
 * -----------
 * A board of R x C boxes has (R+1) x (C+1) dots, numbered from 0.
 * The default board (spec) is 4 x 5 boxes -> dots rows 0..4, cols 0..5.
 *
 *     dot (r,c) --- h-line (r,c) --- dot (r,c+1)
 *        |                             |
 *    v-line (r,c)     box (r,c)    v-line (r,c+1)
 *        |                             |
 *   dot (r+1,c) -- h-line (r+1,c) -- dot (r+1,c+1)
 *
 * A Line is identified by its orientation and its top/left dot.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include <stdbool.h>

#define DEFAULT_ROWS 4 /* boxes per column (spec: 4x5 grid of boxes) */
#define DEFAULT_COLS 5 /* boxes per row */
#define MIN_SIZE 1
#define MAX_SIZE 9 /* keeps every dot index a single digit on screen */

typedef enum { PLAYER_NONE = 0, PLAYER_A = 1, PLAYER_B = 2 } Player;

typedef enum { LINE_HORIZONTAL, LINE_VERTICAL } Orientation;

typedef struct {
  Orientation orient;
  int r; /* row of the line's top/left dot */
  int c; /* column of the line's top/left dot */
} Line;

/* Result of checking a move typed by a player. */
typedef enum {
  MOVE_OK,
  MOVE_OUT_OF_RANGE,  /* a dot is outside the grid */
  MOVE_SAME_DOT,      /* both dots are the same dot */
  MOVE_NOT_ADJACENT,  /* dots are diagonal or more than one step apart */
  MOVE_ALREADY_DRAWN  /* that line already exists */
} MoveStatus;

/* Opaque type: other modules can only use the functions below. */
typedef struct Board Board;

/* Allocates an empty rows x cols board. Returns NULL on bad size or no
 * memory. The caller must release it with board_destroy(). */
Board *board_create(int rows, int cols);
void board_destroy(Board *b);

int board_rows(const Board *b);
int board_cols(const Board *b);

/* Checks the move "dot (r1,c1) to dot (r2,c2)". Dots may be given in any
 * order. On MOVE_OK, *out holds the normalized line. */
MoveStatus board_validate_move(const Board *b, int r1, int c1, int r2,
                               int c2, Line *out);

bool board_has_line(const Board *b, Line line);

/* Draws a line that board_validate_move() accepted, and gives every box it
 * completes to player p. Returns how many boxes were completed (0, 1 or 2).
 * Runs in O(1): only the (at most two) boxes touching the line are checked. */
int board_draw_line(Board *b, Line line, Player p);

/* Number of drawn sides (0..4) around box (r,c). */
int board_box_sides(const Board *b, int r, int c);

/* Owner of box (r,c), or PLAYER_NONE if it is still open. */
Player board_box_owner(const Board *b, int r, int c);

int board_score(const Board *b, Player p);

/* True when every line is drawn (equivalently, every box is claimed). */
bool board_is_full(const Board *b);

#endif /* BOARD_H_ */
