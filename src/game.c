/*
 * game.c - The two-player game loop: turns, bonus turns and the winner.
 */
#include "game.h"

#include <stdbool.h>
#include <stdio.h>

#include "board.h"
#include "display.h"
#include "input.h"

static Player other_player(Player p) {
  return (p == PLAYER_A) ? PLAYER_B : PLAYER_A;
}

/* Keeps asking until the player types a legal move. */
static bool ask_for_move(const Board *b, Player current, Line *out) {
  for (;;) {
    display_prompt(current);
    int r1, c1, r2, c2;
    InputStatus in = input_read_move(&r1, &c1, &r2, &c2);
    if (in == INPUT_EOF) {
      return false;
    }
    if (in == INPUT_BAD_FORMAT) {
      display_bad_format();
      continue;
    }
    MoveStatus status = board_validate_move(b, r1, c1, r2, c2, out);
    if (status == MOVE_OK) {
      return true;
    }
    display_move_error(status);
  }
}

GameResult game_play(int rows, int cols) {
  Board *board = board_create(rows, cols);
  if (board == NULL) {
    fprintf(stderr, "Error: could not create a %dx%d board.\n", rows, cols);
    return GAME_ERROR;
  }

  Player current = PLAYER_A;
  Line last = {LINE_HORIZONTAL, 0, 0};
  bool has_last = false;
  int last_completed = 0;
  GameResult result = GAME_FINISHED;

  /* One turn per loop, until every box is taken. */
  display_title();
  while (!board_is_full(board)) {
    display_clear();
    display_board(board, has_last ? &last : NULL);
    display_separator();
    display_scores(board);
    display_separator();
    if (last_completed > 0) {
      display_bonus_turn(current, last_completed);
    }

    if (!ask_for_move(board, current, &last)) {
      printf("\nInput closed - game aborted.\n");
      result = GAME_ABORTED;
      break;
    }
    has_last = true;

    /* Completing a box means the same player goes again. */
    last_completed = board_draw_line(board, last, current);
    if (last_completed == 0) {
      current = other_player(current);
    }
  }

  /* Game over: show the final board and the winner. */
  if (result == GAME_FINISHED) {
    display_clear();
    display_board(board, has_last ? &last : NULL);
    display_winner(board);
  }
  board_destroy(board);
  return result;
}
