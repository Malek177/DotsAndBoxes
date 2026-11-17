/*
 * board.c - Board state and game rules.
 */
#include "board.h"

#include <assert.h>
#include <stdlib.h>

struct Board {
  int rows;
  int cols;
  unsigned char *h;
  unsigned char *v;
  Player *owner;
  int score[3];
  int lines_drawn;
  int total_lines;
};

/* Turn (row, column) into a position in the 1D arrays. */
static int h_index(const Board *b, int r, int c) { return r * b->cols + c; }
static int v_index(const Board *b, int r, int c) { return r * (b->cols + 1) + c; }
static int box_index(const Board *b, int r, int c) { return r * b->cols + c; }

static bool line_in_range(const Board *b, Line line) {
  if (line.orient == LINE_HORIZONTAL) {
    return line.r >= 0 && line.r <= b->rows && line.c >= 0 && line.c < b->cols;
  }
  return line.r >= 0 && line.r < b->rows && line.c >= 0 && line.c <= b->cols;
}

/* Creating and freeing the board. */
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
  b->h = calloc((size_t)(rows + 1) * cols, sizeof(unsigned char));
  b->v = calloc((size_t)rows * (cols + 1), sizeof(unsigned char));
  b->owner = calloc((size_t)rows * cols, sizeof(Player));
  if (b->h == NULL || b->v == NULL || b->owner == NULL) {
    board_destroy(b);
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

/* Reading the board. */
bool board_has_line(const Board *b, Line line) {
  assert(line_in_range(b, line));
  if (line.orient == LINE_HORIZONTAL) {
    return b->h[h_index(b, line.r, line.c)] != 0;
  }
  return b->v[v_index(b, line.r, line.c)] != 0;
}

int board_box_sides(const Board *b, int r, int c) {
  assert(r >= 0 && r < b->rows && c >= 0 && c < b->cols);
  return b->h[h_index(b, r, c)]
       + b->h[h_index(b, r + 1, c)]
       + b->v[v_index(b, r, c)]
       + b->v[v_index(b, r, c + 1)];
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

/* Checking and making moves. */
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
  if (r1 == r2 && abs(c1 - c2) == 1) {
    line.orient = LINE_HORIZONTAL;
    line.r = r1;
    line.c = (c1 < c2) ? c1 : c2;
  } else if (c1 == c2 && abs(r1 - r2) == 1) {
    line.orient = LINE_VERTICAL;
    line.r = (r1 < r2) ? r1 : r2;
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

static int try_claim(Board *b, int r, int c, Player p) {
  if (board_box_sides(b, r, c) == 4 && b->owner[box_index(b, r, c)] == PLAYER_NONE) {
    b->owner[box_index(b, r, c)] = p;
    b->score[p]++;
    return 1;
  }
  return 0;
}

/* Only the boxes on each side of the new line can be completed. */
int board_draw_line(Board *b, Line line, Player p) {
  assert(p == PLAYER_A || p == PLAYER_B);
  assert(line_in_range(b, line));
  assert(!board_has_line(b, line));

  int completed = 0;
  if (line.orient == LINE_HORIZONTAL) {
    b->h[h_index(b, line.r, line.c)] = 1;
    if (line.r > 0) {
      completed += try_claim(b, line.r - 1, line.c, p);
    }
    if (line.r < b->rows) {
      completed += try_claim(b, line.r, line.c, p);
    }
  } else {
    b->v[v_index(b, line.r, line.c)] = 1;
    if (line.c > 0) {
      completed += try_claim(b, line.r, line.c - 1, p);
    }
    if (line.c < b->cols) {
      completed += try_claim(b, line.r, line.c, p);
    }
  }
  b->lines_drawn++;
  return completed;
}
