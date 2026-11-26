/*
 * bot.h - The hard-level bot: minimax search with alpha-beta pruning.
 */
#ifndef BOT_H_
#define BOT_H_

#include "board.h"

typedef struct {
  long nodes;  /* positions examined */
  int depth;   /* moves looked ahead in the last finished search */
  int value;   /* expected final score difference for the bot */
} BotStats;

/* Picks a free line for `me` without changing the board. stats may be NULL. */
Line bot_choose_move(const Board *b, Player me, BotStats *stats);

#endif
