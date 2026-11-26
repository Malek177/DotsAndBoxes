/*
 * test_bot.c - Tests for the minimax bot.
 * Run with: make test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "bot.h"

static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    checks_run++;                                                      \
    if (!(cond)) {                                                     \
      checks_failed++;                                                 \
      printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
    }                                                                  \
  } while (0)

/* Same games on every run. */
static unsigned long seed;
static int random_below(int n) {
  seed = seed * 1103515245UL + 12345UL;
  return (int)((seed / 65536UL) % (unsigned long)n);
}

static Player other(Player p) { return (p == PLAYER_A) ? PLAYER_B : PLAYER_A; }

static int free_lines(const Board *b, Line out[]) {
  int n = 0;
  for (int r = 0; r <= board_rows(b); r++) {
    for (int c = 0; c < board_cols(b); c++) {
      Line h = {LINE_HORIZONTAL, r, c};
      if (!board_has_line(b, h)) out[n++] = h;
    }
  }
  for (int r = 0; r < board_rows(b); r++) {
    for (int c = 0; c <= board_cols(b); c++) {
      Line v = {LINE_VERTICAL, r, c};
      if (!board_has_line(b, v)) out[n++] = v;
    }
  }
  return n;
}

static bool same_line(Line a, Line b) {
  return a.orient == b.orient && a.r == b.r && a.c == b.c;
}

static Player play(Board *b, Line line, Player turn) {
  return (board_draw_line(b, line, turn) > 0) ? turn : other(turn);
}

/* Plain minimax to the end of the game, no pruning: the exact value. */
static int exact_value(Board *b, Player turn, Player me) {
  if (board_is_full(b)) {
    return board_score(b, me) - board_score(b, other(me));
  }
  Line moves[200];
  int n = free_lines(b, moves);
  int best = (turn == me) ? -1000 : 1000;
  for (int i = 0; i < n; i++) {
    Board *next = board_copy(b);
    Player next_turn = play(next, moves[i], turn);
    int value = exact_value(next, next_turn, me);
    board_destroy(next);
    if (turn == me ? value > best : value < best) {
      best = value;
    }
  }
  return best;
}

static void test_takes_last_box(void) {
  printf("test_takes_last_box\n");
  Board *b = board_create(1, 1);
  Line top = {LINE_HORIZONTAL, 0, 0};
  Line bottom = {LINE_HORIZONTAL, 1, 0};
  Line left = {LINE_VERTICAL, 0, 0};
  Line right = {LINE_VERTICAL, 0, 1};
  board_draw_line(b, top, PLAYER_A);
  board_draw_line(b, bottom, PLAYER_A);
  board_draw_line(b, left, PLAYER_A);
  BotStats stats;
  Line move = bot_choose_move(b, PLAYER_B, &stats);
  CHECK(same_line(move, right));
  CHECK(stats.value == 1);
  board_destroy(b);
}

static void test_takes_free_box(void) {
  printf("test_takes_free_box\n");
  Board *b = board_create(1, 2);
  Line top = {LINE_HORIZONTAL, 0, 0};
  Line bottom = {LINE_HORIZONTAL, 1, 0};
  Line left = {LINE_VERTICAL, 0, 0};
  Line middle = {LINE_VERTICAL, 0, 1};
  board_draw_line(b, top, PLAYER_A);
  board_draw_line(b, bottom, PLAYER_A);
  board_draw_line(b, left, PLAYER_A);
  BotStats stats;
  Line move = bot_choose_move(b, PLAYER_B, &stats);
  CHECK(same_line(move, middle));
  CHECK(stats.value == 2);
  board_destroy(b);
}

/* With few lines left the bot must agree with plain minimax. */
static void test_matches_plain_minimax(void) {
  printf("test_matches_plain_minimax\n");
  seed = 12345;
  int sizes[3][2] = {{2, 2}, {1, 3}, {2, 3}};
  for (int k = 0; k < 3; k++) {
    for (int game = 0; game < 10; game++) {
      Board *b = board_create(sizes[k][0], sizes[k][1]);
      Player turn = PLAYER_A;
      while (board_lines_left(b) > 7) {
        Line moves[200];
        int n = free_lines(b, moves);
        turn = play(b, moves[random_below(n)], turn);
      }
      BotStats stats;
      Line move = bot_choose_move(b, turn, &stats);
      int exact = exact_value(b, turn, turn);
      CHECK(stats.depth == board_lines_left(b));
      CHECK(stats.value == exact);

      Player next = play(b, move, turn);
      CHECK(exact_value(b, next, turn) == exact);
      board_destroy(b);
    }
  }
}

typedef struct {
  bool lines[200];
  Player owners[MAX_SIZE * MAX_SIZE];
  int score_a, score_b;
} Snapshot;

static void snapshot(const Board *b, Snapshot *s) {
  memset(s, 0, sizeof(*s));
  int n = 0;
  for (int r = 0; r <= board_rows(b); r++) {
    for (int c = 0; c <= board_cols(b); c++) {
      Line h = {LINE_HORIZONTAL, r, c};
      Line v = {LINE_VERTICAL, r, c};
      s->lines[n++] = c < board_cols(b) && board_has_line(b, h);
      s->lines[n++] = r < board_rows(b) && board_has_line(b, v);
    }
  }
  n = 0;
  for (int r = 0; r < board_rows(b); r++) {
    for (int c = 0; c < board_cols(b); c++) {
      s->owners[n++] = board_box_owner(b, r, c);
    }
  }
  s->score_a = board_score(b, PLAYER_A);
  s->score_b = board_score(b, PLAYER_B);
}

static bool same_snapshot(const Snapshot *x, const Snapshot *y) {
  for (int i = 0; i < 200; i++) {
    if (x->lines[i] != y->lines[i]) return false;
  }
  for (int i = 0; i < MAX_SIZE * MAX_SIZE; i++) {
    if (x->owners[i] != y->owners[i]) return false;
  }
  return x->score_a == y->score_a && x->score_b == y->score_b;
}

/* Bot moves must be free lines and must not change the real board. */
static void test_games_vs_random(void) {
  printf("test_games_vs_random\n");
  seed = 777;
  int sizes[4][2] = {{1, 1}, {1, 2}, {2, 2}, {1, 3}};
  int wins = 0, losses = 0, games = 0;
  for (int k = 0; k < 4; k++) {
    for (int game = 0; game < 2; game++) {
      Board *b = board_create(sizes[k][0], sizes[k][1]);
      Player turn = (game == 0) ? PLAYER_A : PLAYER_B;
      bool ok = true;
      while (!board_is_full(b)) {
        Line move;
        if (turn == PLAYER_B) {
          Snapshot before, after;
          snapshot(b, &before);
          move = bot_choose_move(b, PLAYER_B, NULL);
          snapshot(b, &after);
          ok = ok && same_snapshot(&before, &after);
          ok = ok && !board_has_line(b, move);
        } else {
          Line moves[200];
          int n = free_lines(b, moves);
          move = moves[random_below(n)];
        }
        turn = play(b, move, turn);
      }
      CHECK(ok);
      CHECK(board_score(b, PLAYER_A) + board_score(b, PLAYER_B) ==
            board_rows(b) * board_cols(b));
      if (board_score(b, PLAYER_B) > board_score(b, PLAYER_A)) wins++;
      if (board_score(b, PLAYER_B) < board_score(b, PLAYER_A)) losses++;
      games++;
      board_destroy(b);
    }
  }
  printf("  bot won %d, lost %d of %d games\n", wins, losses, games);
}

int main(void) {
  test_takes_last_box();
  test_takes_free_box();
  test_matches_plain_minimax();
  test_games_vs_random();
  printf("%d checks, %d failed\n", checks_run, checks_failed);
  return checks_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
