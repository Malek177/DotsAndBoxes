/*
 * game.h - The game loop, for two players or one player against the bot.
 */
#ifndef GAME_H_
#define GAME_H_

typedef enum { GAME_FINISHED, GAME_ABORTED, GAME_ERROR } GameResult;

typedef enum { MODE_TWO_PLAYERS, MODE_VS_BOT } GameMode;

/* Against the bot, the human is Player A and moves first. */
GameResult game_play(int rows, int cols, GameMode mode);

#endif
