/*
 * bot_arena.c - Measures the bot: games against a random player and a
 * greedy player on the default 4x5 board, with time and search depth.
 * Run with: make arena
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "board.h"
#include "bot.h"

#define GAMES 10

static unsigned long seed = 2026;
static int random_below(int n) {
  seed = seed * 1103515245UL + 12345UL;
  return (int)((seed / 65536UL) % (unsigned long)n);
}

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

static int sides_around(const Board *b, Line l) {
  int mask = 0;
  if (l.orient == LINE_HORIZONTAL) {
    if (l.r > 0) mask |= 1 << board_box_sides(b, l.r - 1, l.c);
    if (l.r < board_rows(b)) mask |= 1 << board_box_sides(b, l.r, l.c);
  } else {
    if (l.c > 0) mask |= 1 << board_box_sides(b, l.r, l.c - 1);
    if (l.c < board_cols(b)) mask |= 1 << board_box_sides(b, l.r, l.c);
  }
  return mask;
}

/* Takes a box if it can, else a safe line, else any line. */
static Line greedy_move(const Board *b) {
  Line all[200], safe[200];
  int n = free_lines(b, all), ns = 0;
  for (int i = 0; i < n; i++) {
    int mask = sides_around(b, all[i]);
    if (mask & (1 << 3)) return all[i];
    if (!(mask & (1 << 2))) safe[ns++] = all[i];
  }
  return (ns > 0) ? safe[random_below(ns)] : all[random_below(n)];
}

static void run(const char *name, int greedy) {
  int wins = 0, ties = 0, bot_moves = 0;
  double total_time = 0, max_time = 0;
  long stage_nodes[3] = {0};
  int stage_depth[3] = {0}, stage_moves[3] = {0};

  for (int game = 0; game < GAMES; game++) {
    Board *b = board_create(DEFAULT_ROWS, DEFAULT_COLS);
    Player turn = (game % 2 == 0) ? PLAYER_A : PLAYER_B;
    while (!board_is_full(b)) {
      Line move;
      if (turn == PLAYER_B) {
        BotStats stats;
        int left = board_lines_left(b);
        clock_t start = clock();
        move = bot_choose_move(b, PLAYER_B, &stats);
        double t = (double)(clock() - start) / CLOCKS_PER_SEC;
        total_time += t;
        if (t > max_time) max_time = t;
        bot_moves++;
        int stage = (left > 33) ? 0 : (left > 16) ? 1 : 2;
        stage_nodes[stage] += stats.nodes;
        stage_depth[stage] += stats.depth;
        stage_moves[stage]++;
      } else if (greedy) {
        move = greedy_move(b);
      } else {
        Line all[200];
        move = all[random_below(free_lines(b, all))];
      }
      if (board_draw_line(b, move, turn) == 0) {
        turn = (turn == PLAYER_A) ? PLAYER_B : PLAYER_A;
      }
    }
    int a = board_score(b, PLAYER_A), bot = board_score(b, PLAYER_B);
    if (bot > a) wins++;
    if (bot == a) ties++;
    board_destroy(b);
  }

  printf("Bot vs %s (%d games, 4x5): won %d, tied %d, lost %d\n", name, GAMES,
         wins, ties, GAMES - wins - ties);
  printf("  time per move: average %.3f s, slowest %.3f s\n",
         total_time / bot_moves, max_time);
  const char *stages[3] = {"49-34 lines left", "33-17 lines left",
                           "16-1 lines left "};
  for (int s = 0; s < 3; s++) {
    if (stage_moves[s] > 0) {
      printf("  %s: average depth %4.1f, average positions %7ld\n", stages[s],
             (double)stage_depth[s] / stage_moves[s],
             stage_nodes[s] / stage_moves[s]);
    }
  }
}

int main(void) {
  run("random", 0);
  run("greedy", 1);
  return EXIT_SUCCESS;
}
