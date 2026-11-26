/*
 * bot.c - The hard bot: minimax with alpha-beta pruning.
 */
#include "bot.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* Divided by the number of lines: 250,000 positions per move on 4x5. */
#define WORK_BUDGET 12250000L
#define MAX_LINES (2 * MAX_SIZE * (MAX_SIZE + 1))
#define INF 1000000

typedef struct {
  Player me;
  long nodes;
  long budget;
  bool out_of_budget;
} Search;

static Player other(Player p) { return (p == PLAYER_A) ? PLAYER_B : PLAYER_A; }

static int evaluate(const Board *b, Player me) {
  return board_score(b, me) - board_score(b, other(me));
}

/* Boxes this line would complete, and boxes it would leave with 3 sides. */
static void classify(const Board *b, Line line, int *completes, int *gives) {
  int boxes[2][2];
  int n = 0;
  if (line.orient == LINE_HORIZONTAL) {
    if (line.r > 0) {
      boxes[n][0] = line.r - 1;
      boxes[n++][1] = line.c;
    }
    if (line.r < board_rows(b)) {
      boxes[n][0] = line.r;
      boxes[n++][1] = line.c;
    }
  } else {
    if (line.c > 0) {
      boxes[n][0] = line.r;
      boxes[n++][1] = line.c - 1;
    }
    if (line.c < board_cols(b)) {
      boxes[n][0] = line.r;
      boxes[n++][1] = line.c;
    }
  }
  *completes = 0;
  *gives = 0;
  for (int i = 0; i < n; i++) {
    int sides = board_box_sides(b, boxes[i][0], boxes[i][1]);
    if (sides == 3) {
      (*completes)++;
    } else if (sides == 2) {
      (*gives)++;
    }
  }
}

/* Free lines, best first: captures, then safe moves, then risky ones. */
static int list_moves(const Board *b, Line out[]) {
  Line captures[MAX_LINES], safe[MAX_LINES], risky[MAX_LINES];
  int nc = 0, ns = 0, nr = 0;
  int rows = board_rows(b), cols = board_cols(b);

  for (int pass = 0; pass < 2; pass++) {
    Orientation orient = (pass == 0) ? LINE_HORIZONTAL : LINE_VERTICAL;
    int max_r = (pass == 0) ? rows : rows - 1;
    int max_c = (pass == 0) ? cols - 1 : cols;
    for (int r = 0; r <= max_r; r++) {
      for (int c = 0; c <= max_c; c++) {
        Line line = {orient, r, c};
        if (board_has_line(b, line)) {
          continue;
        }
        int completes, gives;
        classify(b, line, &completes, &gives);
        if (completes > 0) {
          captures[nc++] = line;
        } else if (gives == 0) {
          safe[ns++] = line;
        } else {
          risky[nr++] = line;
        }
      }
    }
  }

  int n = 0;
  for (int i = 0; i < nc; i++) out[n++] = captures[i];
  for (int i = 0; i < ns; i++) out[n++] = safe[i];
  for (int i = 0; i < nr; i++) out[n++] = risky[i];
  return n;
}

static bool find_capture(const Board *b, Line *out) {
  for (int r = 0; r < board_rows(b); r++) {
    for (int c = 0; c < board_cols(b); c++) {
      if (board_box_sides(b, r, c) != 3) {
        continue;
      }
      Line sides[4] = {{LINE_HORIZONTAL, r, c}, {LINE_HORIZONTAL, r + 1, c},
                       {LINE_VERTICAL, r, c}, {LINE_VERTICAL, r, c + 1}};
      for (int i = 0; i < 4; i++) {
        if (!board_has_line(b, sides[i])) {
          *out = sides[i];
          return true;
        }
      }
    }
  }
  return false;
}

/* At a leaf, the player to move takes every free box before we score it. */
static int settle_and_evaluate(Board *b, Player turn, Player me) {
  Line taken[MAX_LINES];
  int n = 0;
  Line line;
  while (find_capture(b, &line)) {
    board_draw_line(b, line, turn);
    taken[n++] = line;
  }
  int value = evaluate(b, me);
  while (n > 0) {
    board_undo_line(b, taken[--n]);
  }
  return value;
}

/* The bot maximises its score difference, the opponent minimises it. */
static int minimax(Board *b, int depth, int alpha, int beta, Player turn,
                   Search *s) {
  s->nodes++;
  if (s->nodes > s->budget) {
    s->out_of_budget = true;
    return 0;
  }
  if (board_is_full(b)) {
    return evaluate(b, s->me);
  }
  if (depth == 0) {
    return settle_and_evaluate(b, turn, s->me);
  }

  Line moves[MAX_LINES];
  int n = list_moves(b, moves);
  bool maximizing = (turn == s->me);
  int best = maximizing ? -INF : INF;

  for (int i = 0; i < n; i++) {
    int completed = board_draw_line(b, moves[i], turn);
    Player next = (completed > 0) ? turn : other(turn);
    int value = minimax(b, depth - 1, alpha, beta, next, s);
    board_undo_line(b, moves[i]);
    if (s->out_of_budget) {
      return 0;
    }

    if (maximizing) {
      if (value > best) best = value;
      if (best > alpha) alpha = best;
    } else {
      if (value < best) best = value;
      if (best < beta) beta = best;
    }
    if (alpha >= beta) {
      break;
    }
  }
  return best;
}

/* Iterative deepening: depth 1, 2, 3, ... until the budget runs out. */
Line bot_choose_move(const Board *board, Player me, BotStats *stats) {
  Board *b = board_copy(board);
  assert(b != NULL);

  Line moves[MAX_LINES];
  int n = list_moves(b, moves);
  assert(n > 0);

  int rows = board_rows(b), cols = board_cols(b);
  long total_lines = (long)(rows + 1) * cols + (long)rows * (cols + 1);
  Search s = {me, 0, WORK_BUDGET / total_lines, false};
  Line best = moves[0];
  int best_value = 0;
  int depth_done = 0;

  for (int depth = 1; depth <= board_lines_left(b); depth++) {
    long nodes_before = s.nodes;
    int alpha = -INF;
    int best_index = 0;
    int depth_value = -INF;

    for (int i = 0; i < n; i++) {
      int completed = board_draw_line(b, moves[i], me);
      Player next = (completed > 0) ? me : other(me);
      int value = minimax(b, depth - 1, alpha, INF, next, &s);
      board_undo_line(b, moves[i]);
      if (s.out_of_budget) {
        break;
      }
      if (value > depth_value) {
        depth_value = value;
        best_index = i;
      }
      if (value > alpha) {
        alpha = value;
      }
    }
    if (s.out_of_budget) {
      break;
    }

    best = moves[best_index];
    best_value = depth_value;
    depth_done = depth;

    for (int i = best_index; i > 0; i--) {
      moves[i] = moves[i - 1];
    }
    moves[0] = best;

    /* The next depth costs at least twice this one. */
    long used = s.nodes - nodes_before;
    if (s.nodes + 2 * used > s.budget) {
      break;
    }
  }

  board_destroy(b);
  if (stats != NULL) {
    stats->nodes = s.nodes;
    stats->depth = depth_done;
    stats->value = best_value;
  }
  return best;
}
