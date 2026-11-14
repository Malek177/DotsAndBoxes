/*
 * board.c - Implementation of the board state and rules (see board.h).
 */
#include "board.h"

#include <assert.h>
#include <stdlib.h>

struct Board {
  int rows;              /* number of box rows    (dots: rows + 1) */
  int cols;              /* number of box columns (dots: cols + 1) */
  unsigned char *h;      /* (rows+1) x cols  horizontal lines, 1 = drawn */
  unsigned char *v;      /* rows x (cols+1)  vertical lines,   1 = drawn */
  Player *owner;         /* rows x cols      box owners */
  int score[3];          /* indexed by Player; score[PLAYER_NONE] unused */
  int lines_drawn;
  int total_lines;
};

/* ---- index helpers (2D -> 1D, row-major) -------------------------- */

static int h_index(const Board *b, int r, int c) { return r * b->cols + c; }
static int v_index(const Board *b, int r, int c) { return r * (b->cols + 1) + c; }
static int box_index(const Board *b, int r, int c) { return r * b->cols + c; }

static bool line_in_range(const Board *b, Line line) {
  if (line.orient == LINE_HORIZONTAL) {
    return line.r >= 0 && line.r <= b->rows && line.c >= 0 && line.c < b->cols;
  }
  return line.r >= 0 && line.r < b->rows && line.c >= 0 && line.c <= b->cols;
}

/* ---- creation / destruction ---------------------------------------- */

Board *board_create(int rows, int cols) {
  if (rows < MIN_SIZE || rows > MAX_SIZE || cols < MIN_SIZE || cols > MAX_SIZE) {
    return NULL;
  }
  Board *b = malloc(sizeof(Board));
  if (b == NULL) {
    return NULL;
  }
  b->rows = rows;
  b->cols = cols;
  /* calloc zero-fills: no lines drawn, every owner = PLAYER_NONE (0). */
  b->h = calloc((size_t)(rows + 1) * cols, sizeof(unsigned char));
  b->v = calloc((size_t)rows * (cols + 1), sizeof(unsigned char));
  b->owner = calloc((size_t)rows * cols, sizeof(Player));
  if (b->h == NULL || b->v == NULL || b->owner == NULL) {
    board_destroy(b); /* free(NULL) is safe, so partial failure is fine */
    return NULL;
  }
  b->score[PLAYER_NONE] = 0;
  b->score[PLAYER_A] = 0;
  b->score[PLAYER_B] = 0;
  b->lines_drawn = 0;
  b->total_lines = (rows + 1) * cols + rows * (cols + 1);
  return b;
}

void board_destroy(Board *b) {
  if (b == NULL) {
    return;
  }
  free(b->h);
  free(b->v);
  free(b->owner);
  free(b);
}

int board_rows(const Board *b) { return b->rows; }
int board_cols(const Board *b) { return b->cols; }

/* ---- queries --------------------------------------------------------- */

bool board_has_line(const Board *b, Line line) {
  assert(line_in_range(b, line));
  if (line.orient == LINE_HORIZONTAL) {
    return b->h[h_index(b, line.r, line.c)] != 0;
  }
  return b->v[v_index(b, line.r, line.c)] != 0;
}

int board_box_sides(const Board *b, int r, int c) {
  assert(r >= 0 && r < b->rows && c >= 0 && c < b->cols);
  return b->h[h_index(b, r, c)]          /* top    */
       + b->h[h_index(b, r + 1, c)]      /* bottom */
       + b->v[v_index(b, r, c)]          /* left   */
       + b->v[v_index(b, r, c + 1)];     /* right  */
}

Player board_box_owner(const Board *b, int r, int c) {
  assert(r >= 0 && r < b->rows && c >= 0 && c < b->cols);
  return b->owner[box_index(b, r, c)];
}

int board_score(const Board *b, Player p) {
  assert(p == PLAYER_A || p == PLAYER_B);
  return b->score[p];
}

bool board_is_full(const Board *b) { return b->lines_drawn == b->total_lines; }

/* ---- moves ------------------------------------------------------------ */

static bool dot_in_range(const Board *b, int r, int c) {
  return r >= 0 && r <= b->rows && c >= 0 && c <= b->cols;
}

MoveStatus board_validate_move(const Board *b, int r1, int c1, int r2,
                               int c2, Line *out) {
  if (!dot_in_range(b, r1, c1) || !dot_in_range(b, r2, c2)) {
    return MOVE_OUT_OF_RANGE;
  }
  if (r1 == r2 && c1 == c2) {
    return MOVE_SAME_DOT;
  }

  Line line;
  if (r1 == r2 && abs(c1 - c2) == 1) {          /* same row, neighbours */
    line.orient = LINE_HORIZONTAL;
    line.r = r1;
    line.c = (c1 < c2) ? c1 : c2;               /* left dot */
  } else if (c1 == c2 && abs(r1 - r2) == 1) {   /* same column, neighbours */
    line.orient = LINE_VERTICAL;
    line.r = (r1 < r2) ? r1 : r2;               /* top dot */
    line.c = c1;
  } else {
    return MOVE_NOT_ADJACENT;
  }

  if (board_has_line(b, line)) {
    return MOVE_ALREADY_DRAWN;
  }
  if (out != NULL) {
    *out = line;
  }
  return MOVE_OK;
}

/* Gives box (r,c) to p if all four sides are now drawn. Returns 1 if it did. */
static int try_claim(Board *b, int r, int c, Player p) {
  if (board_box_sides(b, r, c) == 4 && b->owner[box_index(b, r, c)] == PLAYER_NONE) {
    b->owner[box_index(b, r, c)] = p;
    b->score[p]++;
    return 1;
  }
  return 0;
}

int board_draw_line(Board *b, Line line, Player p) {
  assert(p == PLAYER_A || p == PLAYER_B);
  assert(line_in_range(b, line));
  assert(!board_has_line(b, line));

  int completed = 0;
  if (line.orient == LINE_HORIZONTAL) {
    b->h[h_index(b, line.r, line.c)] = 1;
    if (line.r > 0) {          /* box above the line */
      completed += try_claim(b, line.r - 1, line.c, p);
    }
    if (line.r < b->rows) {    /* box below the line */
      completed += try_claim(b, line.r, line.c, p);
    }
  } else {
    b->v[v_index(b, line.r, line.c)] = 1;
    if (line.c > 0) {          /* box left of the line */
      completed += try_claim(b, line.r, line.c - 1, p);
    }
    if (line.c < b->cols) {    /* box right of the line */
      completed += try_claim(b, line.r, line.c, p);
    }
  }
  b->lines_drawn++;
  return completed;
}
