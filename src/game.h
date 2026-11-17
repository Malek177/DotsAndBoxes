/*
 * game.h - The two-player game loop.
 */
#ifndef GAME_H_
#define GAME_H_

typedef enum { GAME_FINISHED, GAME_ABORTED, GAME_ERROR } GameResult;

GameResult game_play(int rows, int cols);

#endif
