/*
 * display.h - Everything the game prints to the terminal.
 *
 * Two styles:
 *   - fancy (default): ANSI colours, screen is cleared before each turn,
 *     the last line drawn is highlighted. Works on the Linux text console.
 *   - plain (--plain): no escape codes at all, output scrolls like the
 *     sample run in the spec. Used by the automated tests and Valgrind runs.
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>

#include "board.h"

/* fancy: colours, bigger centred board. rows/cols: board size in boxes. */
void display_init(bool fancy, int rows, int cols);

/* Single-letter name of a player: 'A' or 'B'. */
char display_player_char(Player p);

/* Fancy mode: clears the screen and redraws the title. Plain mode: no-op. */
void display_clear(void);
void display_title(void);

/* Prints the grid. last may be NULL; otherwise that line is highlighted. */
void display_board(const Board *b, const Line *last);

void display_scores(const Board *b);
void display_separator(void);
void display_prompt(Player p);

/* Explains why a move was rejected. */
void display_move_error(MoveStatus status);
void display_bad_format(void);

/* "Player A completed 2 boxes and plays again!" */
void display_bonus_turn(Player p, int boxes);

void display_winner(const Board *b);

/* "Play again? [Y/n]: " lined up with the board. */
void display_play_again(void);

#endif /* DISPLAY_H_ */
