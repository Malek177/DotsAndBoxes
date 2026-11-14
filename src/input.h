/*
 * input.h - Reading and parsing what the players type.
 *
 * Input is read one whole line at a time with fgets() and then parsed with
 * strtol(), so bad input (letters, too few numbers, huge numbers, very long
 * lines) can never crash the game or leave junk in the input stream.
 */
#ifndef INPUT_H_
#define INPUT_H_

#include <stdbool.h>

typedef enum {
  INPUT_OK,
  INPUT_BAD_FORMAT, /* line was not exactly what we asked for */
  INPUT_EOF         /* end of input (Ctrl+D or end of a test file) */
} InputStatus;

/* Reads "r1 c1 r2 c2" (spaces or commas between numbers). */
InputStatus input_read_move(int *r1, int *c1, int *r2, int *c2);

/* Reads a y/n answer. Empty line or anything starting with y/Y = yes. */
InputStatus input_read_yes_no(bool *yes);

/* Pure parser used by input_read_move (exposed for unit tests).
 * Returns true if line holds exactly 4 integers. */
bool input_parse_move(const char *line, int out[4]);

#endif /* INPUT_H_ */
