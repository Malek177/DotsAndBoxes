/*
 * display.h - Everything the game prints to the terminal.
 * Fancy mode: colours, bigger board centred on screen.
 * Plain mode (--plain): the small layout from the spec, used by the tests.
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>

#include "board.h"

void display_init(bool fancy, int rows, int cols);

void display_set_bot_game(bool vs_bot);
char display_player_char(Player p);

void display_clear(void);
void display_title(void);
void display_board(const Board *b, const Line *last);
void display_scores(const Board *b);
void display_separator(void);
void display_prompt(Player p);

void display_move_error(MoveStatus status);
void display_bad_format(void);
void display_bonus_turn(Player p, int boxes);
void display_winner(const Board *b);
void display_play_again(void);

void display_menu(void);
void display_bad_choice(void);
void display_bot_thinking(void);
void display_bot_move(Line line);

#endif
