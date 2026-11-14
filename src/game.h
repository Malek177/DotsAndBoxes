/*
 * game.h - The two-player game loop (turns, scoring, winner).
 */
#ifndef GAME_H_
#define GAME_H_

typedef enum {
  GAME_FINISHED, /* all boxes claimed, winner announced */
  GAME_ABORTED,  /* input ended (EOF) before the game was over */
  GAME_ERROR     /* could not allocate the board */
} GameResult;

/* Plays one full game of Player A vs Player B on a rows x cols board. */
GameResult game_play(int rows, int cols);

#endif /* GAME_H_ */
