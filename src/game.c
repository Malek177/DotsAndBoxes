/*
 * game.c - The game loop: turns, bonus turns, bot moves and the winner.
 */
#include "game.h"

#include <stdbool.h>
#include <stdio.h>

#include "board.h"
#include "bot.h"
#include "display.h"
#include "input.h"

#define BOT_PLAYER PLAYER_B

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

GameResult game_play(int rows, int cols, GameMode mode) {
  Board *board = board_create(rows, cols);
  if (board == NULL) {
    fprintf(stderr, "Error: could not create a %dx%d board.\n", rows, cols);
    return GAME_ERROR;
  }

  Player current = PLAYER_A;
  Line last = {LINE_HORIZONTAL, 0, 0};
  bool has_last = false;
  int last_completed = 0;
  bool bot_moved = false;
  GameResult result = GAME_FINISHED;
  display_set_bot_game(mode == MODE_VS_BOT);

  /* One turn per loop, until every box is taken. */
  display_title();
  while (!board_is_full(board)) {
    display_clear();
    display_board(board, has_last ? &last : NULL);
    display_separator();
    display_scores(board);
    display_separator();
    if (bot_moved) {
      display_bot_move(last);
    }
    if (last_completed > 0) {
      display_bonus_turn(current, last_completed);
    }

    if (mode == MODE_VS_BOT && current == BOT_PLAYER) {
      display_bot_thinking();
      last = bot_choose_move(board, BOT_PLAYER, NULL);
      if (board_has_line(board, last)) {
        fprintf(stderr, "Error: the bot chose a line that is already drawn.\n");
        result = GAME_ERROR;
        break;
      }
      bot_moved = true;
    } else {
      if (!ask_for_move(board, current, &last)) {
        printf("\nInput closed - game aborted.\n");
        result = GAME_ABORTED;
        break;
      }
      bot_moved = false;
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
    if (bot_moved) {
      display_bot_move(last);
    }
    display_winner(board);
  }
  board_destroy(board);
  return result;
}
